#!/usr/bin/env python3
"""
FT812 / FT81x DXT1_L2_RGB565 and DXT1_L4_RGB565 image converter.

This script intentionally does not use the old FTDI png2dxt1 two-L1-layer
layout. It encodes the source image directly into L2/L4 raw layouts, DXT
block dumps, and DXP zlib containers compatible with the current L2/L4 viewer.

FT812 display-list idea for the generated RAM_G layout:
  - upload raw as: c0 layer + c1 layer + L2/L4 mask
  - color bitmap handle points to RGB565 color cells
  - cell 0 = c0 layer, cell 1 = c1 layer
  - mask bitmap handle points to the L2/L4 mask
  - first draw the mask so it writes destination alpha
  - then draw c1 with DST_ALPHA
  - then draw c0 with ONE_MINUS_DST_ALPHA

DXP compatibility note:
  - current viewer DXP uses an 8-byte header: "DXP" + type + width + height
  - type 0 = raw L2, type 1 = zlib L2, type 2 = zlib L4, type 3 = raw L4

Examples:
  python3 -m pip install pillow
  python3 ft812_dxt_convert_v2.py input.png -o out -f l2,l4 -t raw,dxt,dxp -e 10 -j 0 -z 9 -p -x
  python3 ft812_dxt_convert_v2.py input.webp -o out -f l2 -t dxp -e 4 -z 9
  python3 ft812_dxt_convert_v2.py input.jpg -o out -f l4 -t raw -e 6 -x -p
"""

from __future__ import annotations

import argparse
import concurrent.futures
import dataclasses
import math
import os
import re
import struct
import sys
import time
import zlib
from pathlib import Path
from typing import Iterable, Sequence

try:
  from PIL import Image
except ImportError as exc:
  raise SystemExit("Pillow is required. Install it with: python3 -m pip install pillow") from exc


@dataclasses.dataclass(frozen=True)
class FormatSpec:
  name: str
  suffix: str
  type_id: int
  magic: bytes
  selector_count: int
  selector_bits: int
  mask_format: str


@dataclasses.dataclass(frozen=True)
class EffortConfig:
  refine_iters: int
  local_radius: int
  local_passes: int
  perceptual: bool
  max_candidates: int
  seam_passes: int
  seam_weight: float
  residual: bool
  residual_strength: float
  residual_clamp: float


@dataclasses.dataclass(slots=True)
class SelectOptions:
  seam_candidates: int = 0
  residual_candidates: int = 0
  early_stop: bool = True
  early_stop_threshold: int = 0
  early_stop_ratio: int = 128
  residual_active: bool = True


@dataclasses.dataclass(slots=True)
class Candidate:
  c0: int
  c1: int
  selectors: bytes
  packed_selectors: bytes
  decoded: bytes
  error: float


def ensure_packed_selectors(cand: Candidate, spec: FormatSpec) -> bytes:
  if not cand.packed_selectors:
    cand.packed_selectors = pack_selectors_for_dxt(cand.selectors, spec)
  return cand.packed_selectors


@dataclasses.dataclass
class EncodedImage:
  spec: FormatSpec
  width: int
  height: int
  blocks_x: int
  blocks_y: int
  blocks: list[Candidate]


@dataclasses.dataclass(slots=True)
class RecordBatch:
  block_indices: list[int] = dataclasses.field(default_factory=list)
  c0s: list[int] = dataclasses.field(default_factory=list)
  c1s: list[int] = dataclasses.field(default_factory=list)

  def append(self, block_index: int, c0: int, c1: int) -> None:
    self.block_indices.append(int(block_index))
    self.c0s.append(int(c0))
    self.c1s.append(int(c1))

  def clear(self) -> None:
    self.block_indices.clear()
    self.c0s.clear()
    self.c1s.clear()

  def __len__(self) -> int:
    return len(self.block_indices)


@dataclasses.dataclass(slots=True)
class RawWorkList:
  block_indices: list[int] = dataclasses.field(default_factory=list)
  c0s: list[int] = dataclasses.field(default_factory=list)
  c1s: list[int] = dataclasses.field(default_factory=list)
  selectors: list[bytes] = dataclasses.field(default_factory=list)
  errors: list[float] = dataclasses.field(default_factory=list)
  active: list[bool] = dataclasses.field(default_factory=list)

  def append(self, block_index: int, c0: int, c1: int, selectors: bytes, error: float, active: bool = True) -> None:
    self.block_indices.append(int(block_index))
    self.c0s.append(int(c0))
    self.c1s.append(int(c1))
    self.selectors.append(selectors)
    self.errors.append(float(error))
    self.active.append(bool(active))

  def __len__(self) -> int:
    return len(self.block_indices)


class RawRetainedPool:
  def __init__(self, block_count: int, limit: int, profile: ProfileMetrics | None = None) -> None:
    self.limit = max(1, int(limit))
    self.profile = profile
    self.keys: list[dict[tuple[int, int, bytes], int]] = [dict() for _ in range(block_count)]
    self.c0s: list[list[int]] = [[] for _ in range(block_count)]
    self.c1s: list[list[int]] = [[] for _ in range(block_count)]
    self.selectors: list[list[bytes]] = [[] for _ in range(block_count)]
    self.errors: list[list[float]] = [[] for _ in range(block_count)]
    self.decoded: list[list[bytes]] = [[] for _ in range(block_count)]

  def __len__(self) -> int:
    return sum(len(v) for v in self.c0s)

  def _worst_slot(self, block_index: int) -> tuple[int, float]:
    errs = self.errors[block_index]
    worst_slot = 0
    worst_error = errs[0]
    for i in range(1, len(errs)):
      if errs[i] > worst_error:
        worst_slot = i
        worst_error = errs[i]
    return worst_slot, worst_error

  def worst_error(self, block_index: int) -> float:
    if len(self.errors[block_index]) < self.limit:
      return float("inf")
    return self._worst_slot(block_index)[1]

  def can_enter(self, block_index: int, error: float) -> bool:
    if len(self.errors[block_index]) < self.limit:
      return True
    return error + 1e-9 < self.worst_error(block_index)

  def insert(self, block_index: int, c0: int, c1: int, selectors: bytes, error: float, decoded: bytes = b"") -> bool:
    block_index = int(block_index)
    c0 = int(c0)
    c1 = int(c1)
    error = float(error)
    key = (c0, c1, selectors)
    block_keys = self.keys[block_index]
    old_slot = block_keys.get(key)
    if old_slot is not None:
      if error + 1e-9 < self.errors[block_index][old_slot]:
        self.errors[block_index][old_slot] = error
        self.decoded[block_index][old_slot] = decoded
        return True
      return False

    if len(self.errors[block_index]) < self.limit:
      slot = len(self.errors[block_index])
      block_keys[key] = slot
      self.c0s[block_index].append(c0)
      self.c1s[block_index].append(c1)
      self.selectors[block_index].append(selectors)
      self.errors[block_index].append(error)
      self.decoded[block_index].append(decoded)
      return True

    worst_slot, worst_error = self._worst_slot(block_index)
    if error + 1e-9 >= worst_error:
      return False

    old_key = (self.c0s[block_index][worst_slot], self.c1s[block_index][worst_slot], self.selectors[block_index][worst_slot])
    block_keys.pop(old_key, None)
    block_keys[key] = worst_slot
    self.c0s[block_index][worst_slot] = c0
    self.c1s[block_index][worst_slot] = c1
    self.selectors[block_index][worst_slot] = selectors
    self.errors[block_index][worst_slot] = error
    self.decoded[block_index][worst_slot] = decoded
    return True

  def sorted_slots(self, block_index: int) -> list[int]:
    return sorted(range(len(self.errors[block_index])), key=lambda i: self.errors[block_index][i])

  def to_work_list(self, profile: ProfileMetrics | None = None, profile_key: str | None = None) -> RawWorkList:
    t0 = time.perf_counter()
    work = RawWorkList()
    for block_index in range(len(self.c0s)):
      for slot in self.sorted_slots(block_index):
        work.append(
          block_index,
          self.c0s[block_index][slot],
          self.c1s[block_index][slot],
          self.selectors[block_index][slot],
          self.errors[block_index][slot],
          True,
        )
    if profile is not None and profile_key is not None:
      profile.add_time(profile_key, time.perf_counter() - t0)
      profile.add_count(profile_key.replace("time.", "count.") + ".items", len(work))
    return work

  def to_candidate_lists(self, blocks: Sequence[bytes], spec: FormatSpec, weights: tuple[float, float, float],
                         profile: ProfileMetrics | None = None) -> list[list[Candidate]]:
    t0 = time.perf_counter()
    result: list[list[Candidate]] = []
    final_count = 0
    for block_index in range(len(self.c0s)):
      items: list[Candidate] = []
      for slot in self.sorted_slots(block_index):
        items.append(Candidate(
          c0=self.c0s[block_index][slot],
          c1=self.c1s[block_index][slot],
          selectors=self.selectors[block_index][slot],
          packed_selectors=b"",
          decoded=self.decoded[block_index][slot],
          error=self.errors[block_index][slot],
        ))
      if not items:
        items = [evaluate_rgb_pair(blocks[block_index], (0, 0, 0), (0, 0, 0), spec, weights)]
      final_count += len(items)
      result.append(items)
    if profile is not None:
      profile.add_time("raw_pool.final_candidate_build", time.perf_counter() - t0)
      profile.add_count("candidate.final_build", final_count)
    return result


@dataclasses.dataclass
class ProfileMetrics:
  enabled: bool = False
  times: dict[str, float] = dataclasses.field(default_factory=dict)
  counts: dict[str, int] = dataclasses.field(default_factory=dict)

  def add_time(self, name: str, seconds: float) -> None:
    if not self.enabled:
      return
    self.times[name] = self.times.get(name, 0.0) + seconds

  def add_count(self, name: str, value: int) -> None:
    if not self.enabled:
      return
    self.counts[name] = self.counts.get(name, 0) + int(value)


  def show(self) -> None:
    if not self.enabled:
      return

    print("profile:", file=sys.stderr)
    for name in sorted(self.times):
      print(f"  time.{name}: {self.times[name]:.6f}s", file=sys.stderr)

    for name in sorted(self.counts):
      print(f"  count.{name}: {self.counts[name]}", file=sys.stderr)

    score_pairs = self.counts.get("gpu.score.pairs", 0)
    score_calls = self.counts.get("gpu.score.calls", 0)
    score_time = self.times.get("gpu.score", 0.0)
    if score_pairs > 0 and score_time > 0.0:
      print(f"  rate.gpu.score: {score_pairs / score_time:.1f} pairs/s", file=sys.stderr)
    if score_calls > 0:
      print(f"  avg.gpu.score_batch: {score_pairs / score_calls:.1f} pairs", file=sys.stderr)

    decode_pairs = self.counts.get("gpu.decode.pairs", 0)
    decode_calls = self.counts.get("gpu.decode.calls", 0)
    decode_time = self.times.get("gpu.decode", 0.0)
    if decode_pairs > 0 and decode_time > 0.0:
      print(f"  rate.gpu.decode: {decode_pairs / decode_time:.1f} pairs/s", file=sys.stderr)
    if decode_calls > 0:
      print(f"  avg.gpu.decode_batch: {decode_pairs / decode_calls:.1f} pairs", file=sys.stderr)

    gpu_write = self.counts.get("gpu.write.bytes", 0)
    gpu_read = self.counts.get("gpu.read.bytes", 0)
    gpu_time = score_time + decode_time
    if gpu_time > 0.0:
      mb = 1024.0 * 1024.0
      print(f"  rate.gpu.transfer: {(gpu_write + gpu_read) / mb / gpu_time:.3f} MiB/s approx", file=sys.stderr)

    encode_total = sum(v for k, v in self.times.items() if k.endswith(".gpu_encode.total"))
    gpu_total = self.times.get("gpu.init", 0.0) + gpu_time
    if encode_total > 0.0:
      overhead = max(0.0, encode_total - gpu_total)
      print(f"  ratio.gpu_time_in_encode: {gpu_total / encode_total * 100.0:.1f}%", file=sys.stderr)
      print(f"  time.python_or_cpu_overhead_est: {overhead:.6f}s", file=sys.stderr)


EFFORT_LEVEL_HELP = """
Effort levels:
  0  Fast min/max: luminance/RGB range endpoints, one selector pass.
  1  PCA/farthest-pair: min/max, farthest-pair and PCA endpoint candidates, light refinement.
  2  Default quality: more endpoint candidates, PCA, 4 refinement iterations, small RGB565 local search.
  3  Strong independent block: 6 refinement iterations, more local endpoint variants, RGB/luma weighted error.
  4  Max independent block: 8 refinement iterations, wider block-local endpoint candidate search.
  5  Perceptual candidate mode: like level 4 with perceptual RGB weighting and more retained candidates.
  6  Seam-aware light: independent encode plus K candidates per block and 1 neighbor seam-selection pass.
  7  Seam-aware medium: larger K and 2 coordinate-descent seam-selection passes.
  8  Seam-aware strong: larger K, 4 serpentine seam-selection passes, stronger seam penalty.
  9  Seam-aware + residual diffusion: like level 8 with weak clamped block-level residual diffusion.
 10  Max experimental: largest K, more global passes, stronger local search and residual diffusion; slowest.

GPU note:
  GPU is enabled by default and uses OpenCL through optional numpy + pyopencl.
  -c / --cpu disables GPU and uses CPU encoding.
  -g / --gpu keeps explicit compatibility with older command lines.
  --gpu-batch controls OpenCL batch size; larger values reduce kernel-launch/readback overhead.
  --profile prints timing/counter metrics to stderr for GPU/CPU bottleneck analysis, including GPU pipeline substages.
  Initial endpoint scoring and hybrid refine use score-only GPU passes, then decode only retained candidates.
  Local endpoint search, endpoint-variation search, and LS refine fitting are generated inside OpenCL kernels in GPU mode to avoid Python-side records.
  Initial endpoint template generation uses OpenCL in GPU mode; candidate selection, seam-aware passes and residual diffusion remain CPU.
  Final RAW layer packing and preview reconstruction are GPU-accelerated when GPU mode is active.
""".strip()

DXP_TYPE_RAW_L2 = 0
DXP_TYPE_ZLIB_L2 = 1
DXP_TYPE_ZLIB_L4 = 2
DXP_TYPE_RAW_L4 = 3

FORMATS = {
  "l2": FormatSpec(
    name="DXT1_L2_RGB565",
    suffix="l2",
    type_id=1,
    magic=b"D1L2",
    selector_count=4,
    selector_bits=2,
    mask_format="FT_L2",
  ),
  "l4": FormatSpec(
    name="DXT1_L4_RGB565",
    suffix="l4",
    type_id=2,
    magic=b"D1L4",
    selector_count=16,
    selector_bits=4,
    mask_format="FT_L4",
  ),
}

L2_ALPHAS = (0, 255, 85, 170)
RGB_WEIGHTS = (1.0, 1.0, 1.0)
PERCEPTUAL_WEIGHTS = (0.299, 0.587, 0.114)


OPENCL_EVAL_KERNEL = r"""
__kernel void eval_pairs(
    __global const uchar *blocks,
    __global const int *block_indices,
    __global const ushort *c0s,
    __global const ushort *c1s,
    __global uchar *selectors_out,
    __global uchar *decoded_out,
    __global float *errors_out,
    const int pair_count,
    const int selector_count,
    const float wr,
    const float wg,
    const float wb)
{
    int gid = get_global_id(0);
    if (gid >= pair_count) return;

    int block_idx = block_indices[gid];
    __global const uchar *block = blocks + block_idx * 48;

    int c0v = (int)c0s[gid];
    int c1v = (int)c1s[gid];

    int c0r5 = (c0v >> 11) & 31;
    int c0g6 = (c0v >> 5) & 63;
    int c0b5 = c0v & 31;
    int c1r5 = (c1v >> 11) & 31;
    int c1g6 = (c1v >> 5) & 63;
    int c1b5 = c1v & 31;

    int c0r = (c0r5 << 3) | (c0r5 >> 2);
    int c0g = (c0g6 << 2) | (c0g6 >> 4);
    int c0b = (c0b5 << 3) | (c0b5 >> 2);
    int c1r = (c1r5 << 3) | (c1r5 >> 2);
    int c1g = (c1g6 << 2) | (c1g6 >> 4);
    int c1b = (c1b5 << 3) | (c1b5 >> 2);

    float total = 0.0f;

    for (int pix = 0; pix < 16; pix++) {
        int sr = (int)block[pix * 3 + 0];
        int sg = (int)block[pix * 3 + 1];
        int sb = (int)block[pix * 3 + 2];

        int best_sel = 0;
        int best_r = c0r;
        int best_g = c0g;
        int best_b = c0b;
        float best_err = 3.402823e38f;

        for (int sel = 0; sel < selector_count; sel++) {
            int alpha;
            if (selector_count == 4) {
                alpha = (sel == 0) ? 0 : ((sel == 1) ? 255 : ((sel == 2) ? 85 : 170));
            } else {
                alpha = (sel * 255 + 7) / 15;
            }

            int inv = 255 - alpha;
            int pr = (c0r * inv + c1r * alpha + 127) / 255;
            int pg = (c0g * inv + c1g * alpha + 127) / 255;
            int pb = (c0b * inv + c1b * alpha + 127) / 255;

            float dr = (float)(sr - pr);
            float dg = (float)(sg - pg);
            float db = (float)(sb - pb);
            float err = wr * dr * dr + wg * dg * dg + wb * db * db;
            if (err < best_err) {
                best_err = err;
                best_sel = sel;
                best_r = pr;
                best_g = pg;
                best_b = pb;
            }
        }

        selectors_out[gid * 16 + pix] = (uchar)best_sel;
        decoded_out[gid * 48 + pix * 3 + 0] = (uchar)best_r;
        decoded_out[gid * 48 + pix * 3 + 1] = (uchar)best_g;
        decoded_out[gid * 48 + pix * 3 + 2] = (uchar)best_b;
        total += best_err;
    }

    errors_out[gid] = total;
}

__kernel void eval_pair_errors(
    __global const uchar *blocks,
    __global const int *block_indices,
    __global const ushort *c0s,
    __global const ushort *c1s,
    __global float *errors_out,
    const int pair_count,
    const int selector_count,
    const float wr,
    const float wg,
    const float wb)
{
    int gid = get_global_id(0);
    if (gid >= pair_count) return;

    int block_idx = block_indices[gid];
    __global const uchar *block = blocks + block_idx * 48;

    int c0v = (int)c0s[gid];
    int c1v = (int)c1s[gid];

    int c0r5 = (c0v >> 11) & 31;
    int c0g6 = (c0v >> 5) & 63;
    int c0b5 = c0v & 31;
    int c1r5 = (c1v >> 11) & 31;
    int c1g6 = (c1v >> 5) & 63;
    int c1b5 = c1v & 31;

    int c0r = (c0r5 << 3) | (c0r5 >> 2);
    int c0g = (c0g6 << 2) | (c0g6 >> 4);
    int c0b = (c0b5 << 3) | (c0b5 >> 2);
    int c1r = (c1r5 << 3) | (c1r5 >> 2);
    int c1g = (c1g6 << 2) | (c1g6 >> 4);
    int c1b = (c1b5 << 3) | (c1b5 >> 2);

    float total = 0.0f;

    for (int pix = 0; pix < 16; pix++) {
        int sr = (int)block[pix * 3 + 0];
        int sg = (int)block[pix * 3 + 1];
        int sb = (int)block[pix * 3 + 2];
        float best_err = 3.402823e38f;

        for (int sel = 0; sel < selector_count; sel++) {
            int alpha;
            if (selector_count == 4) {
                alpha = (sel == 0) ? 0 : ((sel == 1) ? 255 : ((sel == 2) ? 85 : 170));
            } else {
                alpha = (sel * 255 + 7) / 15;
            }

            int inv = 255 - alpha;
            int pr = (c0r * inv + c1r * alpha + 127) / 255;
            int pg = (c0g * inv + c1g * alpha + 127) / 255;
            int pb = (c0b * inv + c1b * alpha + 127) / 255;

            float dr = (float)(sr - pr);
            float dg = (float)(sg - pg);
            float db = (float)(sb - pb);
            float err = wr * dr * dr + wg * dg * dg + wb * db * db;
            if (err < best_err) {
                best_err = err;
            }
        }

        total += best_err;
    }

    errors_out[gid] = total;
}


int template_pack565(const int r_in, const int g_in, const int b_in)
{
    int r = r_in;
    int g = g_in;
    int b = b_in;
    if (r < 0) r = 0; if (r > 255) r = 255;
    if (g < 0) g = 0; if (g > 255) g = 255;
    if (b < 0) b = 0; if (b > 255) b = 255;
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

void template_add_pair(
    __global ushort *out_c0s,
    __global ushort *out_c1s,
    const int base,
    __private int *count,
    const int max_pairs,
    const int c0,
    const int c1)
{
    if (*count >= max_pairs) return;
    for (int i = 0; i < *count; i++) {
        if ((int)out_c0s[base + i] == c0 && (int)out_c1s[base + i] == c1) return;
    }
    out_c0s[base + *count] = (ushort)c0;
    out_c1s[base + *count] = (ushort)c1;
    *count = *count + 1;
}

__kernel void initial_template_pairs(
    __global const uchar *unique_blocks,
    __global ushort *out_c0s,
    __global ushort *out_c1s,
    __global int *out_counts,
    const int unique_count,
    const int max_pairs,
    const int effort,
    const float wr,
    const float wg,
    const float wb)
{
    int gid = get_global_id(0);
    if (gid >= unique_count) return;

    __global const uchar *block = unique_blocks + gid * 48;
    int base = gid * max_pairs;
    int count = 0;

    int r[16];
    int g[16];
    int b[16];
    int lum[16];

    int min_r = 255;
    int min_g = 255;
    int min_b = 255;
    int max_r = 0;
    int max_g = 0;
    int max_b = 0;
    int min_lum = 2147483647;
    int max_lum = -1;
    int min_lum_i = 0;
    int max_lum_i = 0;

    float mean_r = 0.0f;
    float mean_g = 0.0f;
    float mean_b = 0.0f;

    for (int pix = 0; pix < 16; pix++) {
        int rr = (int)block[pix * 3 + 0];
        int gg = (int)block[pix * 3 + 1];
        int bb = (int)block[pix * 3 + 2];
        int ll = rr * 299 + gg * 587 + bb * 114;
        r[pix] = rr;
        g[pix] = gg;
        b[pix] = bb;
        lum[pix] = ll;
        if (rr < min_r) min_r = rr;
        if (gg < min_g) min_g = gg;
        if (bb < min_b) min_b = bb;
        if (rr > max_r) max_r = rr;
        if (gg > max_g) max_g = gg;
        if (bb > max_b) max_b = bb;
        if (ll < min_lum) { min_lum = ll; min_lum_i = pix; }
        if (ll > max_lum) { max_lum = ll; max_lum_i = pix; }
        mean_r += (float)rr;
        mean_g += (float)gg;
        mean_b += (float)bb;
    }

    mean_r *= 1.0f / 16.0f;
    mean_g *= 1.0f / 16.0f;
    mean_b *= 1.0f / 16.0f;

    int min_rgb = template_pack565(min_r, min_g, min_b);
    int max_rgb = template_pack565(max_r, max_g, max_b);
    template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, min_rgb, max_rgb);
    template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, max_rgb, min_rgb);

    int lum_min_rgb = template_pack565(r[min_lum_i], g[min_lum_i], b[min_lum_i]);
    int lum_max_rgb = template_pack565(r[max_lum_i], g[max_lum_i], b[max_lum_i]);
    template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, lum_min_rgb, lum_max_rgb);
    template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, lum_max_rgb, lum_min_rgb);

    if (effort >= 1) {
        float best_dist = -1.0f;
        int best_a = 0;
        int best_b = 0;
        for (int a = 0; a < 16; a++) {
            for (int c = 0; c < 16; c++) {
                float dr = (float)(r[a] - r[c]);
                float dg = (float)(g[a] - g[c]);
                float db = (float)(b[a] - b[c]);
                float dist = wr * dr * dr + wg * dg * dg + wb * db * db;
                if (dist > best_dist) {
                    best_dist = dist;
                    best_a = a;
                    best_b = c;
                }
            }
        }
        int far0 = template_pack565(r[best_a], g[best_a], b[best_a]);
        int far1 = template_pack565(r[best_b], g[best_b], b[best_b]);
        template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, far0, far1);
        template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, far1, far0);

        float c00 = 0.0f;
        float c01 = 0.0f;
        float c02 = 0.0f;
        float c11 = 0.0f;
        float c12 = 0.0f;
        float c22 = 0.0f;
        for (int pix = 0; pix < 16; pix++) {
            float dr = (float)r[pix] - mean_r;
            float dg = (float)g[pix] - mean_g;
            float db = (float)b[pix] - mean_b;
            c00 += dr * dr;
            c01 += dr * dg;
            c02 += dr * db;
            c11 += dg * dg;
            c12 += dg * db;
            c22 += db * db;
        }

        float v0 = 0.577350269f;
        float v1 = 0.577350269f;
        float v2 = 0.577350269f;
        for (int iter = 0; iter < 8; iter++) {
            float n0 = c00 * v0 + c01 * v1 + c02 * v2;
            float n1 = c01 * v0 + c11 * v1 + c12 * v2;
            float n2 = c02 * v0 + c12 * v1 + c22 * v2;
            float norm = sqrt(n0 * n0 + n1 * n1 + n2 * n2);
            if (norm < 1.0e-9f) break;
            v0 = n0 / norm;
            v1 = n1 / norm;
            v2 = n2 / norm;
        }

        float min_proj = 3.402823e38f;
        float max_proj = -3.402823e38f;
        int min_proj_i = 0;
        int max_proj_i = 0;
        for (int pix = 0; pix < 16; pix++) {
            float proj = ((float)r[pix] - mean_r) * v0 + ((float)g[pix] - mean_g) * v1 + ((float)b[pix] - mean_b) * v2;
            if (proj < min_proj) { min_proj = proj; min_proj_i = pix; }
            if (proj > max_proj) { max_proj = proj; max_proj_i = pix; }
        }

        int pca_idx0 = template_pack565(r[min_proj_i], g[min_proj_i], b[min_proj_i]);
        int pca_idx1 = template_pack565(r[max_proj_i], g[max_proj_i], b[max_proj_i]);
        int pca_axis0 = template_pack565(
            (int)floor(mean_r + v0 * min_proj + 0.5f),
            (int)floor(mean_g + v1 * min_proj + 0.5f),
            (int)floor(mean_b + v2 * min_proj + 0.5f));
        int pca_axis1 = template_pack565(
            (int)floor(mean_r + v0 * max_proj + 0.5f),
            (int)floor(mean_g + v1 * max_proj + 0.5f),
            (int)floor(mean_b + v2 * max_proj + 0.5f));
        template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, pca_idx0, pca_idx1);
        template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, pca_idx1, pca_idx0);
        template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, pca_axis0, pca_axis1);
        template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, pca_axis1, pca_axis0);
    }

    if (effort >= 2) {
        int mean_rgb = template_pack565((int)floor(mean_r + 0.5f), (int)floor(mean_g + 0.5f), (int)floor(mean_b + 0.5f));
        template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, mean_rgb, max_rgb);
        template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, min_rgb, mean_rgb);

        int min_ch[3] = {0, 0, 0};
        int max_ch[3] = {0, 0, 0};
        for (int pix = 1; pix < 16; pix++) {
            if (r[pix] < r[min_ch[0]]) min_ch[0] = pix;
            if (r[pix] > r[max_ch[0]]) max_ch[0] = pix;
            if (g[pix] < g[min_ch[1]]) min_ch[1] = pix;
            if (g[pix] > g[max_ch[1]]) max_ch[1] = pix;
            if (b[pix] < b[min_ch[2]]) min_ch[2] = pix;
            if (b[pix] > b[max_ch[2]]) max_ch[2] = pix;
        }
        for (int ch = 0; ch < 3; ch++) {
            int low = min_ch[ch];
            int high = max_ch[ch];
            int lo_rgb = template_pack565(r[low], g[low], b[low]);
            int hi_rgb = template_pack565(r[high], g[high], b[high]);
            template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, lo_rgb, hi_rgb);
            template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, hi_rgb, lo_rgb);
        }
    }

    if (effort >= 4) {
        int low_i[3] = {0, 0, 0};
        int low_v[3] = {2147483647, 2147483647, 2147483647};
        int high_i[3] = {0, 0, 0};
        int high_v[3] = {-1, -1, -1};
        for (int pix = 0; pix < 16; pix++) {
            int ll = lum[pix];
            for (int k = 0; k < 3; k++) {
                if (ll < low_v[k]) {
                    for (int m = 2; m > k; m--) { low_v[m] = low_v[m - 1]; low_i[m] = low_i[m - 1]; }
                    low_v[k] = ll; low_i[k] = pix;
                    break;
                }
            }
            for (int k = 0; k < 3; k++) {
                if (ll > high_v[k]) {
                    for (int m = 2; m > k; m--) { high_v[m] = high_v[m - 1]; high_i[m] = high_i[m - 1]; }
                    high_v[k] = ll; high_i[k] = pix;
                    break;
                }
            }
        }
        int anchors[6];
        for (int k = 0; k < 3; k++) anchors[k] = low_i[k];
        for (int k = 0; k < 3; k++) anchors[3 + k] = high_i[k];
        for (int a = 0; a < 6; a++) {
            for (int c = 0; c < 6; c++) {
                int ai = anchors[a];
                int ci = anchors[c];
                if (r[ai] == r[ci] && g[ai] == g[ci] && b[ai] == b[ci]) continue;
                int ar = template_pack565(r[ai], g[ai], b[ai]);
                int cr = template_pack565(r[ci], g[ci], b[ci]);
                template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, ar, cr);
            }
        }
    }

    if (effort >= 5) {
        int min_ch[3] = {0, 0, 0};
        int max_ch[3] = {0, 0, 0};
        for (int pix = 1; pix < 16; pix++) {
            if (r[pix] < r[min_ch[0]]) min_ch[0] = pix;
            if (r[pix] > r[max_ch[0]]) max_ch[0] = pix;
            if (g[pix] < g[min_ch[1]]) min_ch[1] = pix;
            if (g[pix] > g[max_ch[1]]) max_ch[1] = pix;
            if (b[pix] < b[min_ch[2]]) min_ch[2] = pix;
            if (b[pix] > b[max_ch[2]]) max_ch[2] = pix;
        }
        for (int ch = 0; ch < 3; ch++) {
            int low = min_ch[ch];
            int high = max_ch[ch];
            int lo_rgb = template_pack565(r[low], g[low], b[low]);
            int hi_rgb = template_pack565(r[high], g[high], b[high]);
            template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, lo_rgb, hi_rgb);
            template_add_pair(out_c0s, out_c1s, base, &count, max_pairs, hi_rgb, lo_rgb);
        }
    }

    out_counts[gid] = count;
}


__kernel void local_search_best(
    __global const uchar *blocks,
    __global const int *block_indices,
    __global const ushort *c0s,
    __global const ushort *c1s,
    __global const float *current_errors,
    __global ushort *best_c0s,
    __global ushort *best_c1s,
    __global float *best_errors,
    const int item_count,
    const int selector_count,
    const int radius,
    const int passes,
    const float wr,
    const float wg,
    const float wb)
{
    int gid = get_global_id(0);
    if (gid >= item_count) return;

    int block_idx = block_indices[gid];
    __global const uchar *block = blocks + block_idx * 48;

    int best_c0 = (int)c0s[gid];
    int best_c1 = (int)c1s[gid];
    float best_total = current_errors[gid];

    int limits[6] = {31, 63, 31, 31, 63, 31};

    for (int pass = 0; pass < passes; pass++) {
        int improved = 0;

        for (int coord = 0; coord < 6; coord++) {
            for (int step = 1; step <= radius; step++) {
                for (int direction_i = 0; direction_i < 2; direction_i++) {
                    int direction = (direction_i == 0) ? -1 : 1;

                    int p0 = (best_c0 >> 11) & 31;
                    int p1 = (best_c0 >> 5) & 63;
                    int p2 = best_c0 & 31;
                    int p3 = (best_c1 >> 11) & 31;
                    int p4 = (best_c1 >> 5) & 63;
                    int p5 = best_c1 & 31;

                    int old_value = 0;
                    if (coord == 0) old_value = p0;
                    else if (coord == 1) old_value = p1;
                    else if (coord == 2) old_value = p2;
                    else if (coord == 3) old_value = p3;
                    else if (coord == 4) old_value = p4;
                    else old_value = p5;

                    int new_value = old_value + direction * step;
                    if (new_value < 0) new_value = 0;
                    if (new_value > limits[coord]) new_value = limits[coord];
                    if (new_value == old_value) continue;

                    if (coord == 0) p0 = new_value;
                    else if (coord == 1) p1 = new_value;
                    else if (coord == 2) p2 = new_value;
                    else if (coord == 3) p3 = new_value;
                    else if (coord == 4) p4 = new_value;
                    else p5 = new_value;

                    int test_c0 = (p0 << 11) | (p1 << 5) | p2;
                    int test_c1 = (p3 << 11) | (p4 << 5) | p5;

                    int c0r5 = (test_c0 >> 11) & 31;
                    int c0g6 = (test_c0 >> 5) & 63;
                    int c0b5 = test_c0 & 31;
                    int c1r5 = (test_c1 >> 11) & 31;
                    int c1g6 = (test_c1 >> 5) & 63;
                    int c1b5 = test_c1 & 31;

                    int c0r = (c0r5 << 3) | (c0r5 >> 2);
                    int c0g = (c0g6 << 2) | (c0g6 >> 4);
                    int c0b = (c0b5 << 3) | (c0b5 >> 2);
                    int c1r = (c1r5 << 3) | (c1r5 >> 2);
                    int c1g = (c1g6 << 2) | (c1g6 >> 4);
                    int c1b = (c1b5 << 3) | (c1b5 >> 2);

                    float total = 0.0f;
                    for (int pix = 0; pix < 16; pix++) {
                        int sr = (int)block[pix * 3 + 0];
                        int sg = (int)block[pix * 3 + 1];
                        int sb = (int)block[pix * 3 + 2];
                        float best_err = 3.402823e38f;

                        for (int sel = 0; sel < selector_count; sel++) {
                            int alpha;
                            if (selector_count == 4) {
                                alpha = (sel == 0) ? 0 : ((sel == 1) ? 255 : ((sel == 2) ? 85 : 170));
                            } else {
                                alpha = (sel * 255 + 7) / 15;
                            }

                            int inv = 255 - alpha;
                            int pr = (c0r * inv + c1r * alpha + 127) / 255;
                            int pg = (c0g * inv + c1g * alpha + 127) / 255;
                            int pb = (c0b * inv + c1b * alpha + 127) / 255;

                            float dr = (float)(sr - pr);
                            float dg = (float)(sg - pg);
                            float db = (float)(sb - pb);
                            float err = wr * dr * dr + wg * dg * dg + wb * db * db;
                            if (err < best_err) best_err = err;
                        }
                        total += best_err;
                    }

                    if (total + 1.0e-6f < best_total) {
                        best_total = total;
                        best_c0 = test_c0;
                        best_c1 = test_c1;
                        improved = 1;
                    }
                }
            }
        }

        if (!improved) break;
    }

    best_c0s[gid] = (ushort)best_c0;
    best_c1s[gid] = (ushort)best_c1;
    best_errors[gid] = best_total;
}

float eval_pair_total(
    __global const uchar *block,
    const int test_c0,
    const int test_c1,
    const int selector_count,
    const float wr,
    const float wg,
    const float wb)
{
    int c0r5 = (test_c0 >> 11) & 31;
    int c0g6 = (test_c0 >> 5) & 63;
    int c0b5 = test_c0 & 31;
    int c1r5 = (test_c1 >> 11) & 31;
    int c1g6 = (test_c1 >> 5) & 63;
    int c1b5 = test_c1 & 31;

    int c0r = (c0r5 << 3) | (c0r5 >> 2);
    int c0g = (c0g6 << 2) | (c0g6 >> 4);
    int c0b = (c0b5 << 3) | (c0b5 >> 2);
    int c1r = (c1r5 << 3) | (c1r5 >> 2);
    int c1g = (c1g6 << 2) | (c1g6 >> 4);
    int c1b = (c1b5 << 3) | (c1b5 >> 2);

    float total = 0.0f;
    for (int pix = 0; pix < 16; pix++) {
        int sr = (int)block[pix * 3 + 0];
        int sg = (int)block[pix * 3 + 1];
        int sb = (int)block[pix * 3 + 2];
        float best_err = 3.402823e38f;

        for (int sel = 0; sel < selector_count; sel++) {
            int alpha;
            if (selector_count == 4) {
                alpha = (sel == 0) ? 0 : ((sel == 1) ? 255 : ((sel == 2) ? 85 : 170));
            } else {
                alpha = (sel * 255 + 7) / 15;
            }

            int inv = 255 - alpha;
            int pr = (c0r * inv + c1r * alpha + 127) / 255;
            int pg = (c0g * inv + c1g * alpha + 127) / 255;
            int pb = (c0b * inv + c1b * alpha + 127) / 255;

            float dr = (float)(sr - pr);
            float dg = (float)(sg - pg);
            float db = (float)(sb - pb);
            float err = wr * dr * dr + wg * dg * dg + wb * db * db;
            if (err < best_err) best_err = err;
        }
        total += best_err;
    }
    return total;
}

__kernel void variation_search_best(
    __global const uchar *blocks,
    __global const int *block_indices,
    __global const ushort *c0s,
    __global const ushort *c1s,
    __global const float *current_errors,
    __global ushort *best_c0s,
    __global ushort *best_c1s,
    __global float *best_errors,
    const int item_count,
    const int selector_count,
    const int radius,
    const int effort,
    const int max_extra,
    const float wr,
    const float wg,
    const float wb)
{
    int gid = get_global_id(0);
    if (gid >= item_count) return;

    int block_idx = block_indices[gid];
    __global const uchar *block = blocks + block_idx * 48;

    int base_c0 = (int)c0s[gid];
    int base_c1 = (int)c1s[gid];
    int best_c0 = base_c0;
    int best_c1 = base_c1;
    float best_total = current_errors[gid];
    int found = 0;
    int emitted = 0;

    int limits[6] = {31, 63, 31, 31, 63, 31};
    int base_parts[6];
    base_parts[0] = (base_c0 >> 11) & 31;
    base_parts[1] = (base_c0 >> 5) & 63;
    base_parts[2] = base_c0 & 31;
    base_parts[3] = (base_c1 >> 11) & 31;
    base_parts[4] = (base_c1 >> 5) & 63;
    base_parts[5] = base_c1 & 31;

    for (int coord = 0; coord < 6; coord++) {
        for (int step = 1; step <= radius; step++) {
            for (int direction_i = 0; direction_i < 2; direction_i++) {
                if (emitted >= max_extra) continue;
                int direction = (direction_i == 0) ? -1 : 1;
                int parts[6];
                for (int i = 0; i < 6; i++) parts[i] = base_parts[i];
                int next_value = parts[coord] + direction * step;
                if (next_value < 0) next_value = 0;
                if (next_value > limits[coord]) next_value = limits[coord];
                if (next_value == parts[coord]) continue;
                parts[coord] = next_value;

                int test_c0 = (parts[0] << 11) | (parts[1] << 5) | parts[2];
                int test_c1 = (parts[3] << 11) | (parts[4] << 5) | parts[5];
                float total = eval_pair_total(block, test_c0, test_c1, selector_count, wr, wg, wb);
                emitted++;
                if (!found || total < best_total) {
                    found = 1;
                    best_total = total;
                    best_c0 = test_c0;
                    best_c1 = test_c1;
                }
            }
        }
    }

    if (effort >= 4) {
        for (int c_a = 0; c_a < 6; c_a++) {
            for (int c_b = c_a + 1; c_b < 6; c_b++) {
                for (int da_i = 0; da_i < 2; da_i++) {
                    for (int db_i = 0; db_i < 2; db_i++) {
                        if (emitted >= max_extra) continue;
                        int d_a = (da_i == 0) ? -1 : 1;
                        int d_b = (db_i == 0) ? -1 : 1;
                        int parts[6];
                        for (int i = 0; i < 6; i++) parts[i] = base_parts[i];
                        int va = parts[c_a] + d_a;
                        int vb = parts[c_b] + d_b;
                        if (va < 0) va = 0;
                        if (va > limits[c_a]) va = limits[c_a];
                        if (vb < 0) vb = 0;
                        if (vb > limits[c_b]) vb = limits[c_b];
                        if (va == parts[c_a] && vb == parts[c_b]) continue;
                        parts[c_a] = va;
                        parts[c_b] = vb;

                        int test_c0 = (parts[0] << 11) | (parts[1] << 5) | parts[2];
                        int test_c1 = (parts[3] << 11) | (parts[4] << 5) | parts[5];
                        float total = eval_pair_total(block, test_c0, test_c1, selector_count, wr, wg, wb);
                        emitted++;
                        if (!found || total < best_total) {
                            found = 1;
                            best_total = total;
                            best_c0 = test_c0;
                            best_c1 = test_c1;
                        }
                    }
                }
            }
        }
    }

    best_c0s[gid] = (ushort)best_c0;
    best_c1s[gid] = (ushort)best_c1;
    best_errors[gid] = best_total;
}

__kernel void refine_fit_best(
    __global const uchar *blocks,
    __global const int *block_indices,
    __global const uchar *selectors_in,
    __global const ushort *c0s,
    __global const ushort *c1s,
    __global const float *current_errors,
    __global ushort *best_c0s,
    __global ushort *best_c1s,
    __global float *best_errors,
    const int item_count,
    const int selector_count,
    const float wr,
    const float wg,
    const float wb)
{
    int gid = get_global_id(0);
    if (gid >= item_count) return;

    int block_idx = block_indices[gid];
    __global const uchar *block = blocks + block_idx * 48;
    __global const uchar *sels = selectors_in + gid * 16;

    float a00 = 0.0f;
    float a01 = 0.0f;
    float a11 = 0.0f;
    float b00 = 0.0f;
    float b01 = 0.0f;
    float b02 = 0.0f;
    float b10 = 0.0f;
    float b11 = 0.0f;
    float b12 = 0.0f;
    float sum_r = 0.0f;
    float sum_g = 0.0f;
    float sum_b = 0.0f;

    for (int pix = 0; pix < 16; pix++) {
        int sel = (int)sels[pix];
        int alpha;
        if (selector_count == 4) {
            alpha = (sel == 0) ? 0 : ((sel == 1) ? 255 : ((sel == 2) ? 85 : 170));
        } else {
            alpha = (sel * 255 + 7) / 15;
        }

        float t = (float)alpha * (1.0f / 255.0f);
        float w1 = t;
        float w0 = 1.0f - t;
        float sr = (float)((int)block[pix * 3 + 0]);
        float sg = (float)((int)block[pix * 3 + 1]);
        float sb = (float)((int)block[pix * 3 + 2]);

        a00 += w0 * w0;
        a01 += w0 * w1;
        a11 += w1 * w1;
        b00 += w0 * sr;
        b01 += w0 * sg;
        b02 += w0 * sb;
        b10 += w1 * sr;
        b11 += w1 * sg;
        b12 += w1 * sb;
        sum_r += sr;
        sum_g += sg;
        sum_b += sb;
    }

    float c0r;
    float c0g;
    float c0b;
    float c1r;
    float c1g;
    float c1b;
    float det = a00 * a11 - a01 * a01;
    if (fabs(det) < 1.0e-9f) {
        c0r = sum_r * (1.0f / 16.0f);
        c0g = sum_g * (1.0f / 16.0f);
        c0b = sum_b * (1.0f / 16.0f);
        c1r = c0r;
        c1g = c0g;
        c1b = c0b;
    } else {
        c0r = (b00 * a11 - b10 * a01) / det;
        c0g = (b01 * a11 - b11 * a01) / det;
        c0b = (b02 * a11 - b12 * a01) / det;
        c1r = (a00 * b10 - a01 * b00) / det;
        c1g = (a00 * b11 - a01 * b01) / det;
        c1b = (a00 * b12 - a01 * b02) / det;
    }

    int r0 = (int)floor(c0r + 0.5f);
    int g0 = (int)floor(c0g + 0.5f);
    int b0 = (int)floor(c0b + 0.5f);
    int r1 = (int)floor(c1r + 0.5f);
    int g1 = (int)floor(c1g + 0.5f);
    int b1 = (int)floor(c1b + 0.5f);

    if (r0 < 0) r0 = 0; if (r0 > 255) r0 = 255;
    if (g0 < 0) g0 = 0; if (g0 > 255) g0 = 255;
    if (b0 < 0) b0 = 0; if (b0 > 255) b0 = 255;
    if (r1 < 0) r1 = 0; if (r1 > 255) r1 = 255;
    if (g1 < 0) g1 = 0; if (g1 > 255) g1 = 255;
    if (b1 < 0) b1 = 0; if (b1 > 255) b1 = 255;

    int fit_c0 = ((r0 & 0xF8) << 8) | ((g0 & 0xFC) << 3) | (b0 >> 3);
    int fit_c1 = ((r1 & 0xF8) << 8) | ((g1 & 0xFC) << 3) | (b1 >> 3);
    float fit_error = eval_pair_total(block, fit_c0, fit_c1, selector_count, wr, wg, wb);

    best_c0s[gid] = (ushort)fit_c0;
    best_c1s[gid] = (ushort)fit_c1;
    best_errors[gid] = fit_error;
}


__kernel void pack_raw_layers(
    __global const ushort *c0s,
    __global const ushort *c1s,
    __global const uchar *selectors,
    __global uchar *raw_out,
    const int block_count,
    const int blocks_x,
    const int blocks_y,
    const int selector_bits)
{
    int gid = get_global_id(0);
    if (gid >= block_count) return;

    int c0_size = block_count * 2;
    int c1_offset = c0_size;
    int mask_offset = c0_size * 2;

    int c0 = (int)c0s[gid];
    int c1 = (int)c1s[gid];
    int color_pos = gid * 2;
    raw_out[color_pos + 0] = (uchar)(c0 & 255);
    raw_out[color_pos + 1] = (uchar)((c0 >> 8) & 255);
    raw_out[c1_offset + color_pos + 0] = (uchar)(c1 & 255);
    raw_out[c1_offset + color_pos + 1] = (uchar)((c1 >> 8) & 255);

    int bx = gid % blocks_x;
    int by = gid / blocks_x;
    __global const uchar *sels = selectors + gid * 16;

    if (selector_bits == 2) {
        int stride = blocks_x;
        for (int py = 0; py < 4; py++) {
            int value = 0;
            for (int px = 0; px < 4; px++) {
                int sel = (int)sels[py * 4 + px] & 3;
                int code = 0;
                if (sel == 0) code = 0;
                else if (sel == 1) code = 3;
                else if (sel == 2) code = 1;
                else code = 2;
                value |= code << (6 - px * 2);
            }
            int dst = mask_offset + (by * 4 + py) * stride + bx;
            raw_out[dst] = (uchar)value;
        }
    } else {
        int stride = blocks_x * 2;
        for (int py = 0; py < 4; py++) {
            int s0 = (int)sels[py * 4 + 0] & 15;
            int s1 = (int)sels[py * 4 + 1] & 15;
            int s2 = (int)sels[py * 4 + 2] & 15;
            int s3 = (int)sels[py * 4 + 3] & 15;
            int dst = mask_offset + (by * 4 + py) * stride + bx * 2;
            raw_out[dst + 0] = (uchar)((s0 << 4) | s1);
            raw_out[dst + 1] = (uchar)((s2 << 4) | s3);
        }
    }
}

__kernel void pack_preview_rgb(
    __global const ushort *c0s,
    __global const ushort *c1s,
    __global const uchar *selectors,
    __global uchar *rgb_out,
    const int block_count,
    const int blocks_x,
    const int selector_count)
{
    int gid = get_global_id(0);
    if (gid >= block_count) return;

    int c0v = (int)c0s[gid];
    int c1v = (int)c1s[gid];

    int c0r5 = (c0v >> 11) & 31;
    int c0g6 = (c0v >> 5) & 63;
    int c0b5 = c0v & 31;
    int c1r5 = (c1v >> 11) & 31;
    int c1g6 = (c1v >> 5) & 63;
    int c1b5 = c1v & 31;

    int c0r = (c0r5 << 3) | (c0r5 >> 2);
    int c0g = (c0g6 << 2) | (c0g6 >> 4);
    int c0b = (c0b5 << 3) | (c0b5 >> 2);
    int c1r = (c1r5 << 3) | (c1r5 >> 2);
    int c1g = (c1g6 << 2) | (c1g6 >> 4);
    int c1b = (c1b5 << 3) | (c1b5 >> 2);

    int bx = gid % blocks_x;
    int by = gid / blocks_x;
    int width = blocks_x * 4;
    __global const uchar *sels = selectors + gid * 16;

    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            int sel = (int)sels[py * 4 + px];
            int alpha;
            if (selector_count == 4) {
                alpha = (sel == 0) ? 0 : ((sel == 1) ? 255 : ((sel == 2) ? 85 : 170));
            } else {
                alpha = (sel * 255 + 7) / 15;
            }
            int inv = 255 - alpha;
            int pr = (c0r * inv + c1r * alpha + 127) / 255;
            int pg = (c0g * inv + c1g * alpha + 127) / 255;
            int pb = (c0b * inv + c1b * alpha + 127) / 255;
            int out_pos = ((by * 4 + py) * width + bx * 4 + px) * 3;
            rgb_out[out_pos + 0] = (uchar)pr;
            rgb_out[out_pos + 1] = (uchar)pg;
            rgb_out[out_pos + 2] = (uchar)pb;
        }
    }
}

"""


class CliError(Exception):
  pass


class OpenCLBlockEvaluator:
  def __init__(self, blocks: Sequence[bytes], profile: ProfileMetrics | None = None) -> None:
    self.profile = profile
    try:
      import numpy as np  # type: ignore[import-not-found]
      import pyopencl as cl  # type: ignore[import-not-found]
    except ImportError as exc:
      raise CliError("--gpu requires optional packages: python3 -m pip install numpy pyopencl") from exc

    try:
      init_t0 = time.perf_counter()
      self.np = np
      self.cl = cl
      self.ctx = cl.create_some_context(interactive=False)
      self.queue = cl.CommandQueue(self.ctx)
      self.program = cl.Program(self.ctx, OPENCL_EVAL_KERNEL).build()
      self.kernel_eval_pairs = cl.Kernel(self.program, "eval_pairs")
      self.kernel_eval_pair_errors = cl.Kernel(self.program, "eval_pair_errors")
      self.kernel_local_search_best = cl.Kernel(self.program, "local_search_best")
      self.kernel_variation_search_best = cl.Kernel(self.program, "variation_search_best")
      self.kernel_refine_fit_best = cl.Kernel(self.program, "refine_fit_best")
      self.kernel_initial_template_pairs = cl.Kernel(self.program, "initial_template_pairs")
      self.kernel_pack_raw_layers = cl.Kernel(self.program, "pack_raw_layers")
      self.kernel_pack_preview_rgb = cl.Kernel(self.program, "pack_preview_rgb")
      self.device_name = ", ".join(device.name.strip() for device in self.ctx.devices)
      mf = cl.mem_flags
      joined = b"".join(blocks)
      self.blocks_buf = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=np.frombuffer(joined, dtype=np.uint8))
      if self.profile is not None:
        self.profile.add_time("gpu.init", time.perf_counter() - init_t0)
        self.profile.add_count("gpu.static_upload.bytes", len(joined))
    except Exception as exc:
      raise CliError(f"--gpu OpenCL initialization failed: {exc}") from exc


  def build_initial_templates(self, unique_blocks: Sequence[bytes], effort: int,
                              weights: tuple[float, float, float], max_pairs: int = 64) -> list[list[tuple[int, int]]]:
    count = len(unique_blocks)
    if count == 0:
      return []

    t0 = time.perf_counter()
    np = self.np
    cl = self.cl
    mf = cl.mem_flags

    joined = b"".join(unique_blocks)
    block_bytes = np.frombuffer(joined, dtype=np.uint8)
    out_c0s = np.zeros((count * max_pairs,), dtype=np.uint16)
    out_c1s = np.zeros((count * max_pairs,), dtype=np.uint16)
    out_counts = np.zeros((count,), dtype=np.int32)

    buf_blocks = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=block_bytes)
    buf_c0s = cl.Buffer(self.ctx, mf.WRITE_ONLY, out_c0s.nbytes)
    buf_c1s = cl.Buffer(self.ctx, mf.WRITE_ONLY, out_c1s.nbytes)
    buf_counts = cl.Buffer(self.ctx, mf.WRITE_ONLY, out_counts.nbytes)

    self.kernel_initial_template_pairs(
      self.queue,
      (count,),
      None,
      buf_blocks,
      buf_c0s,
      buf_c1s,
      buf_counts,
      np.int32(count),
      np.int32(max_pairs),
      np.int32(effort),
      np.float32(weights[0]),
      np.float32(weights[1]),
      np.float32(weights[2]),
    )
    cl.enqueue_copy(self.queue, out_c0s, buf_c0s)
    cl.enqueue_copy(self.queue, out_c1s, buf_c1s)
    cl.enqueue_copy(self.queue, out_counts, buf_counts)
    self.queue.finish()

    result: list[list[tuple[int, int]]] = []
    generated = 0
    for idx in range(count):
      start = idx * max_pairs
      n = max(0, min(max_pairs, int(out_counts[idx])))
      pairs: list[tuple[int, int]] = []
      seen: set[tuple[int, int]] = set()
      for off in range(n):
        pair = (int(out_c0s[start + off]), int(out_c1s[start + off]))
        if pair not in seen:
          seen.add(pair)
          pairs.append(pair)
      if not pairs:
        pairs.append((0, 0))
      generated += len(pairs)
      result.append(pairs)

    if self.profile is not None:
      self.profile.add_time("gpu.initial_template.kernel", time.perf_counter() - t0)
      self.profile.add_count("gpu.initial_template.kernel_blocks", count)
      self.profile.add_count("gpu.initial_template.kernel_pairs", generated)
      self.profile.add_count("gpu.write.bytes", block_bytes.nbytes)
      self.profile.add_count("gpu.read.bytes", out_c0s.nbytes + out_c1s.nbytes + out_counts.nbytes)
    return result

  def evaluate_errors_arrays(self, block_indices_in: Sequence[int], c0s_in: Sequence[int], c1s_in: Sequence[int],
                             spec: FormatSpec, weights: tuple[float, float, float]) -> list[float]:
    count = len(block_indices_in)
    if count == 0:
      return []

    t0 = time.perf_counter()
    np = self.np
    cl = self.cl
    mf = cl.mem_flags

    block_indices = np.asarray(block_indices_in, dtype=np.int32)
    c0s = np.asarray(c0s_in, dtype=np.uint16)
    c1s = np.asarray(c1s_in, dtype=np.uint16)
    errors = np.empty((count,), dtype=np.float32)

    buf_indices = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=block_indices)
    buf_c0s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c0s)
    buf_c1s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c1s)
    buf_errors = cl.Buffer(self.ctx, mf.WRITE_ONLY, errors.nbytes)

    self.kernel_eval_pair_errors(
      self.queue,
      (count,),
      None,
      self.blocks_buf,
      buf_indices,
      buf_c0s,
      buf_c1s,
      buf_errors,
      np.int32(count),
      np.int32(spec.selector_count),
      np.float32(weights[0]),
      np.float32(weights[1]),
      np.float32(weights[2]),
    )
    cl.enqueue_copy(self.queue, errors, buf_errors)
    self.queue.finish()
    if self.profile is not None:
      self.profile.add_time("gpu.score", time.perf_counter() - t0)
      self.profile.add_count("gpu.score.calls", 1)
      self.profile.add_count("gpu.score.pairs", count)
      self.profile.add_count("gpu.write.bytes", block_indices.nbytes + c0s.nbytes + c1s.nbytes)
      self.profile.add_count("gpu.read.bytes", errors.nbytes)
    return [float(v) for v in errors.tolist()]


  def local_search_best_arrays(self, block_indices_in: Sequence[int], c0s_in: Sequence[int], c1s_in: Sequence[int],
                               current_errors_in: Sequence[float], spec: FormatSpec,
                               weights: tuple[float, float, float], radius: int, passes: int) -> tuple[list[int], list[int], list[float]]:
    count = len(block_indices_in)
    if count == 0:
      return [], [], []

    t0 = time.perf_counter()
    np = self.np
    cl = self.cl
    mf = cl.mem_flags

    block_indices = np.asarray(block_indices_in, dtype=np.int32)
    c0s = np.asarray(c0s_in, dtype=np.uint16)
    c1s = np.asarray(c1s_in, dtype=np.uint16)
    current_errors = np.asarray(current_errors_in, dtype=np.float32)
    best_c0s = np.empty((count,), dtype=np.uint16)
    best_c1s = np.empty((count,), dtype=np.uint16)
    best_errors = np.empty((count,), dtype=np.float32)

    buf_indices = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=block_indices)
    buf_c0s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c0s)
    buf_c1s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c1s)
    buf_current_errors = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=current_errors)
    buf_best_c0s = cl.Buffer(self.ctx, mf.WRITE_ONLY, best_c0s.nbytes)
    buf_best_c1s = cl.Buffer(self.ctx, mf.WRITE_ONLY, best_c1s.nbytes)
    buf_best_errors = cl.Buffer(self.ctx, mf.WRITE_ONLY, best_errors.nbytes)

    self.kernel_local_search_best(
      self.queue,
      (count,),
      None,
      self.blocks_buf,
      buf_indices,
      buf_c0s,
      buf_c1s,
      buf_current_errors,
      buf_best_c0s,
      buf_best_c1s,
      buf_best_errors,
      np.int32(count),
      np.int32(spec.selector_count),
      np.int32(radius),
      np.int32(passes),
      np.float32(weights[0]),
      np.float32(weights[1]),
      np.float32(weights[2]),
    )
    cl.enqueue_copy(self.queue, best_c0s, buf_best_c0s)
    cl.enqueue_copy(self.queue, best_c1s, buf_best_c1s)
    cl.enqueue_copy(self.queue, best_errors, buf_best_errors)
    self.queue.finish()

    if self.profile is not None:
      elapsed = time.perf_counter() - t0
      # This kernel evaluates local endpoint variants internally. The pair
      # count is an upper bound because each work item may stop early when a
      # pass has no improvement.
      max_pair_evals = count * max(0, passes) * 6 * max(0, radius) * 2
      self.profile.add_time("gpu.score", elapsed)
      self.profile.add_time("gpu.local_search", elapsed)
      self.profile.add_count("gpu.score.calls", 1)
      self.profile.add_count("gpu.score.pairs", max_pair_evals)
      self.profile.add_count("gpu.local_search.calls", 1)
      self.profile.add_count("gpu.local_search.seeds", count)
      self.profile.add_count("gpu.local_search.max_pairs", max_pair_evals)
      self.profile.add_count("gpu.write.bytes", block_indices.nbytes + c0s.nbytes + c1s.nbytes + current_errors.nbytes)
      self.profile.add_count("gpu.read.bytes", best_c0s.nbytes + best_c1s.nbytes + best_errors.nbytes)

    return [int(v) for v in best_c0s.tolist()], [int(v) for v in best_c1s.tolist()], [float(v) for v in best_errors.tolist()]

  def variation_search_best_arrays(self, block_indices_in: Sequence[int], c0s_in: Sequence[int], c1s_in: Sequence[int],
                                   current_errors_in: Sequence[float], spec: FormatSpec,
                                   weights: tuple[float, float, float], radius: int, effort: int,
                                   max_extra: int) -> tuple[list[int], list[int], list[float]]:
    count = len(block_indices_in)
    if count == 0:
      return [], [], []

    t0 = time.perf_counter()
    np = self.np
    cl = self.cl
    mf = cl.mem_flags

    block_indices = np.asarray(block_indices_in, dtype=np.int32)
    c0s = np.asarray(c0s_in, dtype=np.uint16)
    c1s = np.asarray(c1s_in, dtype=np.uint16)
    current_errors = np.asarray(current_errors_in, dtype=np.float32)
    best_c0s = np.empty((count,), dtype=np.uint16)
    best_c1s = np.empty((count,), dtype=np.uint16)
    best_errors = np.empty((count,), dtype=np.float32)

    buf_indices = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=block_indices)
    buf_c0s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c0s)
    buf_c1s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c1s)
    buf_current_errors = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=current_errors)
    buf_best_c0s = cl.Buffer(self.ctx, mf.WRITE_ONLY, best_c0s.nbytes)
    buf_best_c1s = cl.Buffer(self.ctx, mf.WRITE_ONLY, best_c1s.nbytes)
    buf_best_errors = cl.Buffer(self.ctx, mf.WRITE_ONLY, best_errors.nbytes)

    self.kernel_variation_search_best(
      self.queue,
      (count,),
      None,
      self.blocks_buf,
      buf_indices,
      buf_c0s,
      buf_c1s,
      buf_current_errors,
      buf_best_c0s,
      buf_best_c1s,
      buf_best_errors,
      np.int32(count),
      np.int32(spec.selector_count),
      np.int32(radius),
      np.int32(effort),
      np.int32(max_extra),
      np.float32(weights[0]),
      np.float32(weights[1]),
      np.float32(weights[2]),
    )
    cl.enqueue_copy(self.queue, best_c0s, buf_best_c0s)
    cl.enqueue_copy(self.queue, best_c1s, buf_best_c1s)
    cl.enqueue_copy(self.queue, best_errors, buf_best_errors)
    self.queue.finish()

    if self.profile is not None:
      elapsed = time.perf_counter() - t0
      max_pair_evals = count * max(0, max_extra)
      self.profile.add_time("gpu.score", elapsed)
      self.profile.add_time("gpu.variation_search", elapsed)
      self.profile.add_count("gpu.score.calls", 1)
      self.profile.add_count("gpu.score.pairs", max_pair_evals)
      self.profile.add_count("gpu.variation_search.calls", 1)
      self.profile.add_count("gpu.variation_search.seeds", count)
      self.profile.add_count("gpu.variation_search.max_pairs", max_pair_evals)
      self.profile.add_count("gpu.write.bytes", block_indices.nbytes + c0s.nbytes + c1s.nbytes + current_errors.nbytes)
      self.profile.add_count("gpu.read.bytes", best_c0s.nbytes + best_c1s.nbytes + best_errors.nbytes)

    return [int(v) for v in best_c0s.tolist()], [int(v) for v in best_c1s.tolist()], [float(v) for v in best_errors.tolist()]

  def refine_fit_best_arrays(self, block_indices_in: Sequence[int], selectors_in: Sequence[bytes], c0s_in: Sequence[int],
                             c1s_in: Sequence[int], current_errors_in: Sequence[float], spec: FormatSpec,
                             weights: tuple[float, float, float]) -> tuple[list[int], list[int], list[float]]:
    count = len(block_indices_in)
    if count == 0:
      return [], [], []

    t0 = time.perf_counter()
    np = self.np
    cl = self.cl
    mf = cl.mem_flags

    block_indices = np.asarray(block_indices_in, dtype=np.int32)
    selectors = np.empty((count, 16), dtype=np.uint8)
    for i, sels in enumerate(selectors_in):
      selectors[i, :] = np.frombuffer(sels, dtype=np.uint8, count=16)
    c0s = np.asarray(c0s_in, dtype=np.uint16)
    c1s = np.asarray(c1s_in, dtype=np.uint16)
    current_errors = np.asarray(current_errors_in, dtype=np.float32)
    best_c0s = np.empty((count,), dtype=np.uint16)
    best_c1s = np.empty((count,), dtype=np.uint16)
    best_errors = np.empty((count,), dtype=np.float32)

    buf_indices = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=block_indices)
    buf_selectors = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=selectors)
    buf_c0s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c0s)
    buf_c1s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c1s)
    buf_current_errors = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=current_errors)
    buf_best_c0s = cl.Buffer(self.ctx, mf.WRITE_ONLY, best_c0s.nbytes)
    buf_best_c1s = cl.Buffer(self.ctx, mf.WRITE_ONLY, best_c1s.nbytes)
    buf_best_errors = cl.Buffer(self.ctx, mf.WRITE_ONLY, best_errors.nbytes)

    self.kernel_refine_fit_best(
      self.queue,
      (count,),
      None,
      self.blocks_buf,
      buf_indices,
      buf_selectors,
      buf_c0s,
      buf_c1s,
      buf_current_errors,
      buf_best_c0s,
      buf_best_c1s,
      buf_best_errors,
      np.int32(count),
      np.int32(spec.selector_count),
      np.float32(weights[0]),
      np.float32(weights[1]),
      np.float32(weights[2]),
    )
    cl.enqueue_copy(self.queue, best_c0s, buf_best_c0s)
    cl.enqueue_copy(self.queue, best_c1s, buf_best_c1s)
    cl.enqueue_copy(self.queue, best_errors, buf_best_errors)
    self.queue.finish()

    if self.profile is not None:
      elapsed = time.perf_counter() - t0
      self.profile.add_time("gpu.score", elapsed)
      self.profile.add_time("gpu.refine_fit", elapsed)
      self.profile.add_count("gpu.score.calls", 1)
      self.profile.add_count("gpu.score.pairs", count)
      self.profile.add_count("gpu.refine_fit.calls", 1)
      self.profile.add_count("gpu.refine_fit.seeds", count)
      self.profile.add_count("gpu.write.bytes", block_indices.nbytes + selectors.nbytes + c0s.nbytes + c1s.nbytes + current_errors.nbytes)
      self.profile.add_count("gpu.read.bytes", best_c0s.nbytes + best_c1s.nbytes + best_errors.nbytes)

    return [int(v) for v in best_c0s.tolist()], [int(v) for v in best_c1s.tolist()], [float(v) for v in best_errors.tolist()]


  def evaluate_records_raw_arrays(self, block_indices_in: Sequence[int], c0s_in: Sequence[int], c1s_in: Sequence[int],
                                  spec: FormatSpec, weights: tuple[float, float, float],
                                  include_decoded: bool = True) -> tuple[list[int], list[int], list[int], list[bytes], list[float], list[bytes]]:
    count = len(block_indices_in)
    if count == 0:
      return [], [], [], [], [], []

    np = self.np
    cl = self.cl
    mf = cl.mem_flags

    block_indices = np.asarray(block_indices_in, dtype=np.int32)
    c0s = np.asarray(c0s_in, dtype=np.uint16)
    c1s = np.asarray(c1s_in, dtype=np.uint16)
    selectors = np.empty((count, 16), dtype=np.uint8)
    decoded = np.empty((count, 48), dtype=np.uint8)
    errors = np.empty((count,), dtype=np.float32)

    t0 = time.perf_counter()
    buf_indices = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=block_indices)
    buf_c0s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c0s)
    buf_c1s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c1s)
    buf_selectors = cl.Buffer(self.ctx, mf.WRITE_ONLY, selectors.nbytes)
    buf_decoded = cl.Buffer(self.ctx, mf.WRITE_ONLY, decoded.nbytes)
    buf_errors = cl.Buffer(self.ctx, mf.WRITE_ONLY, errors.nbytes)

    self.kernel_eval_pairs(
      self.queue,
      (count,),
      None,
      self.blocks_buf,
      buf_indices,
      buf_c0s,
      buf_c1s,
      buf_selectors,
      buf_decoded,
      buf_errors,
      np.int32(count),
      np.int32(spec.selector_count),
      np.float32(weights[0]),
      np.float32(weights[1]),
      np.float32(weights[2]),
    )
    cl.enqueue_copy(self.queue, selectors, buf_selectors)
    if include_decoded:
      cl.enqueue_copy(self.queue, decoded, buf_decoded)
    cl.enqueue_copy(self.queue, errors, buf_errors)
    self.queue.finish()

    if self.profile is not None:
      self.profile.add_time("gpu.decode", time.perf_counter() - t0)
      self.profile.add_count("gpu.decode.calls", 1)
      self.profile.add_count("gpu.decode.pairs", count)
      self.profile.add_count("gpu.write.bytes", block_indices.nbytes + c0s.nbytes + c1s.nbytes)
      read_bytes = selectors.nbytes + errors.nbytes
      if include_decoded:
        read_bytes += decoded.nbytes
      else:
        self.profile.add_count("gpu.decode.skipped_decoded_bytes", decoded.nbytes)
      self.profile.add_count("gpu.read.bytes", read_bytes)

    build_t0 = time.perf_counter()
    selector_list = [selectors[i].tobytes() for i in range(count)]
    decoded_list = [decoded[i].tobytes() for i in range(count)] if include_decoded else [b""] * count
    if self.profile is not None:
      self.profile.add_time("raw_pool.build", time.perf_counter() - build_t0)
      self.profile.add_count("raw_pool.objects", count)
      self.profile.add_count("candidate.intermediate_skipped", count)
    return (
      [int(v) for v in block_indices.tolist()],
      [int(v) for v in c0s.tolist()],
      [int(v) for v in c1s.tolist()],
      selector_list,
      [float(v) for v in errors.tolist()],
      decoded_list,
    )

  def _candidate_arrays_for_output(self, encoded: EncodedImage) -> tuple[object, object, object]:
    np = self.np
    block_count = len(encoded.blocks)
    c0s = np.empty((block_count,), dtype=np.uint16)
    c1s = np.empty((block_count,), dtype=np.uint16)
    selectors = np.empty((block_count, 16), dtype=np.uint8)
    for i, block in enumerate(encoded.blocks):
      c0s[i] = block.c0
      c1s[i] = block.c1
      selectors[i, :] = np.frombuffer(block.selectors, dtype=np.uint8, count=16)
    return c0s, c1s, selectors

  def pack_raw_from_encoded(self, encoded: EncodedImage) -> bytes:
    block_count = len(encoded.blocks)
    if block_count == 0:
      return b""

    t0 = time.perf_counter()
    np = self.np
    cl = self.cl
    mf = cl.mem_flags
    c0s, c1s, selectors = self._candidate_arrays_for_output(encoded)
    color_size = block_count * 2
    if encoded.spec.selector_bits == 2:
      mask_stride = encoded.width // 4
    else:
      mask_stride = encoded.width // 2
    raw_size = color_size * 2 + mask_stride * encoded.height
    raw = np.empty((raw_size,), dtype=np.uint8)

    buf_c0s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c0s)
    buf_c1s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c1s)
    buf_selectors = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=selectors)
    buf_raw = cl.Buffer(self.ctx, mf.WRITE_ONLY, raw.nbytes)

    self.kernel_pack_raw_layers(
      self.queue,
      (block_count,),
      None,
      buf_c0s,
      buf_c1s,
      buf_selectors,
      buf_raw,
      np.int32(block_count),
      np.int32(encoded.blocks_x),
      np.int32(encoded.blocks_y),
      np.int32(encoded.spec.selector_bits),
    )
    cl.enqueue_copy(self.queue, raw, buf_raw)
    self.queue.finish()

    if self.profile is not None:
      self.profile.add_time("gpu.output.raw_pack", time.perf_counter() - t0)
      self.profile.add_count("gpu.output.raw_pack.calls", 1)
      self.profile.add_count("gpu.output.raw_pack.blocks", block_count)
      self.profile.add_count("gpu.write.bytes", c0s.nbytes + c1s.nbytes + selectors.nbytes)
      self.profile.add_count("gpu.read.bytes", raw.nbytes)
    return raw.tobytes()

  def make_preview_from_encoded(self, encoded: EncodedImage) -> Image.Image:
    block_count = len(encoded.blocks)
    if block_count == 0:
      return Image.new("RGB", (encoded.width, encoded.height))

    t0 = time.perf_counter()
    np = self.np
    cl = self.cl
    mf = cl.mem_flags
    c0s, c1s, selectors = self._candidate_arrays_for_output(encoded)
    rgb = np.empty((encoded.width * encoded.height * 3,), dtype=np.uint8)

    buf_c0s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c0s)
    buf_c1s = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=c1s)
    buf_selectors = cl.Buffer(self.ctx, mf.READ_ONLY | mf.COPY_HOST_PTR, hostbuf=selectors)
    buf_rgb = cl.Buffer(self.ctx, mf.WRITE_ONLY, rgb.nbytes)

    self.kernel_pack_preview_rgb(
      self.queue,
      (block_count,),
      None,
      buf_c0s,
      buf_c1s,
      buf_selectors,
      buf_rgb,
      np.int32(block_count),
      np.int32(encoded.blocks_x),
      np.int32(encoded.spec.selector_count),
    )
    cl.enqueue_copy(self.queue, rgb, buf_rgb)
    self.queue.finish()

    if self.profile is not None:
      self.profile.add_time("gpu.output.preview_pack", time.perf_counter() - t0)
      self.profile.add_count("gpu.output.preview_pack.calls", 1)
      self.profile.add_count("gpu.output.preview_pack.blocks", block_count)
      self.profile.add_count("gpu.write.bytes", c0s.nbytes + c1s.nbytes + selectors.nbytes)
      self.profile.add_count("gpu.read.bytes", rgb.nbytes)
    return Image.frombytes("RGB", (encoded.width, encoded.height), rgb.tobytes())


class Progress:
  def __init__(self) -> None:
    self.last_percent: dict[str, int] = {}

  def show(self, label: str, done: int, total: int, force: bool = False) -> None:
    if total <= 0:
      return
    percent = max(0, min(100, int(done * 100 / total)))
    if not force and self.last_percent.get(label) == percent:
      return
    self.last_percent[label] = percent
    end = "\n" if percent >= 100 else ""
    print(f"\r{label}: {percent:3d}%", file=sys.stderr, end=end, flush=True)


def clamp_int(v: float, lo: int = 0, hi: int = 255) -> int:
  if v < lo:
    return lo
  if v > hi:
    return hi
  return int(round(v))


def clamp_float(v: float, lo: float, hi: float) -> float:
  if v < lo:
    return lo
  if v > hi:
    return hi
  return v


def rgb_to_565(rgb: tuple[int, int, int]) -> int:
  r, g, b = rgb
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def rgb565_to_rgb(v: int) -> tuple[int, int, int]:
  r5 = (v >> 11) & 0x1F
  g6 = (v >> 5) & 0x3F
  b5 = v & 0x1F
  r = (r5 << 3) | (r5 >> 2)
  g = (g6 << 2) | (g6 >> 4)
  b = (b5 << 3) | (b5 >> 2)
  return r, g, b


def split_565(v: int) -> list[int]:
  return [(v >> 11) & 0x1F, (v >> 5) & 0x3F, v & 0x1F]


def join_565(parts: Sequence[int]) -> int:
  r5 = max(0, min(31, int(parts[0])))
  g6 = max(0, min(63, int(parts[1])))
  b5 = max(0, min(31, int(parts[2])))
  return (r5 << 11) | (g6 << 5) | b5


def selector_alphas(spec: FormatSpec) -> tuple[int, ...]:
  if spec.selector_count == 4:
    return L2_ALPHAS
  return tuple(int(round(k * 255 / 15)) for k in range(16))


def mix_rgb(c0: tuple[int, int, int], c1: tuple[int, int, int], alpha: int) -> tuple[int, int, int]:
  inv = 255 - alpha
  return (
    (c0[0] * inv + c1[0] * alpha + 127) // 255,
    (c0[1] * inv + c1[1] * alpha + 127) // 255,
    (c0[2] * inv + c1[2] * alpha + 127) // 255,
  )


def block_bytes_to_pixels(block: bytes) -> list[tuple[int, int, int]]:
  return [(block[i], block[i + 1], block[i + 2]) for i in range(0, 48, 3)]


def pack_selectors_for_dxt(selectors: bytes, spec: FormatSpec) -> bytes:
  if spec.selector_bits == 2:
    value = 0
    for i, sel in enumerate(selectors):
      value |= (int(sel) & 0x03) << (i * 2)
    return struct.pack("<I", value)

  out = bytearray(8)
  for i in range(0, 16, 2):
    out[i // 2] = ((selectors[i] & 0x0F) << 4) | (selectors[i + 1] & 0x0F)
  return bytes(out)


def pixels_to_bytes(pixels: Sequence[tuple[int, int, int]]) -> bytes:
  out = bytearray()
  for r, g, b in pixels:
    out.extend((int(r), int(g), int(b)))
  return bytes(out)


def weighted_sq_error(a: tuple[int, int, int], b: tuple[int, int, int], weights: tuple[float, float, float]) -> float:
  dr = a[0] - b[0]
  dg = a[1] - b[1]
  db = a[2] - b[2]
  return weights[0] * dr * dr + weights[1] * dg * dg + weights[2] * db * db


def evaluate_565_pair(block: bytes, c0_565: int, c1_565: int, spec: FormatSpec,
                      weights: tuple[float, float, float]) -> Candidate:
  pixels = block_bytes_to_pixels(block)
  c0 = rgb565_to_rgb(c0_565)
  c1 = rgb565_to_rgb(c1_565)
  palette = [mix_rgb(c0, c1, a) for a in selector_alphas(spec)]
  selectors = bytearray(16)
  decoded_pixels: list[tuple[int, int, int]] = []
  error = 0.0

  for i, pix in enumerate(pixels):
    best_sel = 0
    best_err = float("inf")
    for sel, pal in enumerate(palette):
      err = weighted_sq_error(pix, pal, weights)
      if err < best_err:
        best_err = err
        best_sel = sel
    selectors[i] = best_sel
    decoded_pixels.append(palette[best_sel])
    error += best_err

  selector_bytes = bytes(selectors)
  return Candidate(
    c0=c0_565,
    c1=c1_565,
    selectors=selector_bytes,
    packed_selectors=pack_selectors_for_dxt(selector_bytes, spec),
    decoded=pixels_to_bytes(decoded_pixels),
    error=error,
  )


def evaluate_rgb_pair(block: bytes, c0_rgb: tuple[int, int, int], c1_rgb: tuple[int, int, int],
                      spec: FormatSpec, weights: tuple[float, float, float]) -> Candidate:
  return evaluate_565_pair(block, rgb_to_565(c0_rgb), rgb_to_565(c1_rgb), spec, weights)


def decoded_bytes_from_candidate(cand: Candidate, spec: FormatSpec) -> bytes:
  c0 = rgb565_to_rgb(cand.c0)
  c1 = rgb565_to_rgb(cand.c1)
  palette = [mix_rgb(c0, c1, a) for a in selector_alphas(spec)]
  out = bytearray(48)
  for pix, sel in enumerate(cand.selectors):
    r, g, b = palette[int(sel)]
    base = pix * 3
    out[base + 0] = r
    out[base + 1] = g
    out[base + 2] = b
  return bytes(out)


def ensure_candidate_decoded(cand: Candidate, spec: FormatSpec) -> bytes:
  if not cand.decoded:
    cand.decoded = decoded_bytes_from_candidate(cand, spec)
  return cand.decoded


def fit_endpoints_from_selectors(block: bytes, selectors: bytes, spec: FormatSpec) -> tuple[tuple[int, int, int], tuple[int, int, int]]:
  pixels = block_bytes_to_pixels(block)
  alphas = selector_alphas(spec)

  a00 = 0.0
  a01 = 0.0
  a11 = 0.0
  b0 = [0.0, 0.0, 0.0]
  b1 = [0.0, 0.0, 0.0]

  for pix, sel in zip(pixels, selectors):
    t = alphas[sel] / 255.0
    w0 = 1.0 - t
    w1 = t
    a00 += w0 * w0
    a01 += w0 * w1
    a11 += w1 * w1
    for ch in range(3):
      b0[ch] += w0 * pix[ch]
      b1[ch] += w1 * pix[ch]

  det = a00 * a11 - a01 * a01
  if abs(det) < 1e-9:
    avg = tuple(clamp_int(sum(p[ch] for p in pixels) / len(pixels)) for ch in range(3))
    return avg, avg

  c0 = []
  c1 = []
  for ch in range(3):
    v0 = (b0[ch] * a11 - b1[ch] * a01) / det
    v1 = (a00 * b1[ch] - a01 * b0[ch]) / det
    c0.append(clamp_int(v0))
    c1.append(clamp_int(v1))
  return (c0[0], c0[1], c0[2]), (c1[0], c1[1], c1[2])


def refine_candidate(block: bytes, cand: Candidate, spec: FormatSpec, weights: tuple[float, float, float],
                     iterations: int) -> Candidate:
  best = cand
  for _ in range(iterations):
    c0_rgb, c1_rgb = fit_endpoints_from_selectors(block, best.selectors, spec)
    next_cand = evaluate_rgb_pair(block, c0_rgb, c1_rgb, spec, weights)
    if next_cand.error <= best.error + 1e-9:
      if next_cand.c0 == best.c0 and next_cand.c1 == best.c1 and next_cand.selectors == best.selectors:
        break
      best = next_cand
    else:
      break
  return best


def local_search_candidate(block: bytes, cand: Candidate, spec: FormatSpec, weights: tuple[float, float, float],
                           radius: int, passes: int) -> Candidate:
  if radius <= 0 or passes <= 0:
    return cand

  best = cand
  c0_parts = split_565(best.c0)
  c1_parts = split_565(best.c1)
  limits = [31, 63, 31, 31, 63, 31]

  for _ in range(passes):
    improved = False
    for coord in range(6):
      for step in range(1, radius + 1):
        for direction in (-1, 1):
          parts = c0_parts + c1_parts
          parts[coord] = max(0, min(limits[coord], parts[coord] + direction * step))
          test = evaluate_565_pair(block, join_565(parts[:3]), join_565(parts[3:]), spec, weights)
          if test.error + 1e-9 < best.error:
            best = test
            c0_parts = split_565(best.c0)
            c1_parts = split_565(best.c1)
            improved = True
    if not improved:
      break
  return best


def add_candidate(candidates: dict[tuple[int, int, bytes], Candidate], cand: Candidate) -> None:
  key = (cand.c0, cand.c1, cand.selectors)
  old = candidates.get(key)
  if old is None or cand.error < old.error:
    candidates[key] = cand


def luminance(rgb: tuple[int, int, int]) -> float:
  return 0.299 * rgb[0] + 0.587 * rgb[1] + 0.114 * rgb[2]


def farthest_pair(pixels: Sequence[tuple[int, int, int]], weights: tuple[float, float, float]) -> tuple[tuple[int, int, int], tuple[int, int, int]]:
  best = (pixels[0], pixels[0])
  best_dist = -1.0
  for a in pixels:
    for b in pixels:
      dist = weighted_sq_error(a, b, weights)
      if dist > best_dist:
        best_dist = dist
        best = (a, b)
  return best


def pca_axis(pixels: Sequence[tuple[int, int, int]]) -> tuple[float, float, float]:
  n = len(pixels)
  mean = [sum(p[ch] for p in pixels) / n for ch in range(3)]
  cov = [[0.0, 0.0, 0.0] for _ in range(3)]
  for p in pixels:
    d = [p[ch] - mean[ch] for ch in range(3)]
    for i in range(3):
      for j in range(3):
        cov[i][j] += d[i] * d[j]

  v = [1.0, 1.0, 1.0]
  for _ in range(12):
    nv = [
      cov[0][0] * v[0] + cov[0][1] * v[1] + cov[0][2] * v[2],
      cov[1][0] * v[0] + cov[1][1] * v[1] + cov[1][2] * v[2],
      cov[2][0] * v[0] + cov[2][1] * v[1] + cov[2][2] * v[2],
    ]
    norm = math.sqrt(nv[0] * nv[0] + nv[1] * nv[1] + nv[2] * nv[2])
    if norm < 1e-9:
      return (0.577350269, 0.577350269, 0.577350269)
    v = [nv[0] / norm, nv[1] / norm, nv[2] / norm]
  return (v[0], v[1], v[2])


def pca_pairs(pixels: Sequence[tuple[int, int, int]]) -> list[tuple[tuple[int, int, int], tuple[int, int, int]]]:
  n = len(pixels)
  mean = [sum(p[ch] for p in pixels) / n for ch in range(3)]
  axis = pca_axis(pixels)
  projections = [sum((p[ch] - mean[ch]) * axis[ch] for ch in range(3)) for p in pixels]
  min_i = min(range(len(pixels)), key=lambda i: projections[i])
  max_i = max(range(len(pixels)), key=lambda i: projections[i])
  min_p = projections[min_i]
  max_p = projections[max_i]
  axis_min = tuple(clamp_int(mean[ch] + axis[ch] * min_p) for ch in range(3))
  axis_max = tuple(clamp_int(mean[ch] + axis[ch] * max_p) for ch in range(3))
  return [(pixels[min_i], pixels[max_i]), (axis_min, axis_max)]


def initial_endpoint_pairs(block: bytes, effort: int, weights: tuple[float, float, float]) -> list[tuple[tuple[int, int, int], tuple[int, int, int]]]:
  pixels = block_bytes_to_pixels(block)
  pairs: list[tuple[tuple[int, int, int], tuple[int, int, int]]] = []

  min_rgb = tuple(min(p[ch] for p in pixels) for ch in range(3))
  max_rgb = tuple(max(p[ch] for p in pixels) for ch in range(3))
  pairs.append((min_rgb, max_rgb))
  pairs.append((max_rgb, min_rgb))

  lum_min = min(pixels, key=luminance)
  lum_max = max(pixels, key=luminance)
  pairs.append((lum_min, lum_max))
  pairs.append((lum_max, lum_min))

  if effort >= 1:
    fp = farthest_pair(pixels, weights)
    pairs.append(fp)
    pairs.append((fp[1], fp[0]))
    for pair in pca_pairs(pixels):
      pairs.append(pair)
      pairs.append((pair[1], pair[0]))

  if effort >= 2:
    mean = tuple(clamp_int(sum(p[ch] for p in pixels) / len(pixels)) for ch in range(3))
    pairs.append((mean, max_rgb))
    pairs.append((min_rgb, mean))
    for ch in range(3):
      low = min(pixels, key=lambda p, c=ch: p[c])
      high = max(pixels, key=lambda p, c=ch: p[c])
      pairs.append((low, high))
      pairs.append((high, low))

  if effort >= 4:
    unique = list(dict.fromkeys(pixels))
    unique.sort(key=luminance)
    anchors = unique[:3] + unique[-3:]
    for a in anchors:
      for b in anchors:
        if a != b:
          pairs.append((a, b))

  if effort >= 5:
    unique = list(dict.fromkeys(pixels))
    by_r = (min(unique, key=lambda p: p[0]), max(unique, key=lambda p: p[0]))
    by_g = (min(unique, key=lambda p: p[1]), max(unique, key=lambda p: p[1]))
    by_b = (min(unique, key=lambda p: p[2]), max(unique, key=lambda p: p[2]))
    pairs.extend([by_r, (by_r[1], by_r[0]), by_g, (by_g[1], by_g[0]), by_b, (by_b[1], by_b[0])])

  seen: set[tuple[tuple[int, int, int], tuple[int, int, int]]] = set()
  out = []
  for pair in pairs:
    if pair not in seen:
      seen.add(pair)
      out.append(pair)
  return out


def endpoint_variations(block: bytes, cand: Candidate, spec: FormatSpec, weights: tuple[float, float, float],
                        effort: int, radius: int, max_extra: int) -> list[Candidate]:
  if radius <= 0 or max_extra <= 0:
    return []

  c0 = split_565(cand.c0)
  c1 = split_565(cand.c1)
  base = c0 + c1
  limits = [31, 63, 31, 31, 63, 31]
  variations: list[Candidate] = []

  def try_parts(parts: list[int]) -> None:
    if len(variations) >= max_extra:
      return
    test = evaluate_565_pair(block, join_565(parts[:3]), join_565(parts[3:]), spec, weights)
    variations.append(test)

  for coord in range(6):
    for step in range(1, radius + 1):
      for direction in (-1, 1):
        parts = list(base)
        parts[coord] = max(0, min(limits[coord], parts[coord] + direction * step))
        try_parts(parts)
        if len(variations) >= max_extra:
          return variations

  if effort >= 4:
    for c_a in range(6):
      for c_b in range(c_a + 1, 6):
        for d_a in (-1, 1):
          for d_b in (-1, 1):
            parts = list(base)
            parts[c_a] = max(0, min(limits[c_a], parts[c_a] + d_a))
            parts[c_b] = max(0, min(limits[c_b], parts[c_b] + d_b))
            try_parts(parts)
            if len(variations) >= max_extra:
              return variations

  return variations


def initial_endpoint_pairs_565(block: bytes, effort: int, weights: tuple[float, float, float]) -> list[tuple[int, int]]:
  pairs_565: list[tuple[int, int]] = []
  seen_pairs: set[tuple[int, int]] = set()
  for c0_rgb, c1_rgb in initial_endpoint_pairs(block, effort, weights):
    pair = (rgb_to_565(c0_rgb), rgb_to_565(c1_rgb))
    if pair not in seen_pairs:
      seen_pairs.add(pair)
      pairs_565.append(pair)
  return pairs_565


def build_initial_pair_cache_gpu(blocks: Sequence[bytes], effort: int, weights: tuple[float, float, float],
                                 evaluator: OpenCLBlockEvaluator,
                                 profile: ProfileMetrics | None = None) -> list[list[tuple[int, int]]]:
  # GPU mode uses a compact OpenCL template generator for the expensive initial
  # endpoint seeds. It intentionally mirrors the CPU candidate families but uses
  # bounded fixed-size output and a compact PCA/anchor implementation so Python
  # does not spend seconds walking every unique 4x4 block.
  t0 = time.perf_counter()
  unique_index: dict[bytes, int] = {}
  unique_blocks: list[bytes] = []
  block_to_unique: list[int] = []
  cache_hits = 0

  for block in blocks:
    idx = unique_index.get(block)
    if idx is None:
      idx = len(unique_blocks)
      unique_index[block] = idx
      unique_blocks.append(block)
    else:
      cache_hits += 1
    block_to_unique.append(idx)

  unique_pairs = evaluator.build_initial_templates(unique_blocks, effort, weights)
  out = [unique_pairs[idx] for idx in block_to_unique]
  generated_pairs = sum(len(pairs) for pairs in out)

  if profile is not None:
    profile.add_time("gpu.initial_template.build", time.perf_counter() - t0)
    profile.add_count("gpu.initial_template.unique_blocks", len(unique_blocks))
    profile.add_count("gpu.initial_template.cache_hits", cache_hits)
    profile.add_count("gpu.initial_template.generated_pairs", generated_pairs)
  return out


def effort_config(effort: int) -> EffortConfig:
  if effort == 0:
    return EffortConfig(0, 0, 0, False, 1, 0, 0.0, False, 0.0, 0.0)
  if effort == 1:
    return EffortConfig(2, 0, 0, False, 1, 0, 0.0, False, 0.0, 0.0)
  if effort == 2:
    return EffortConfig(4, 1, 1, False, 3, 0, 0.0, False, 0.0, 0.0)
  if effort == 3:
    return EffortConfig(6, 1, 2, False, 4, 0, 0.0, False, 0.0, 0.0)
  if effort == 4:
    return EffortConfig(8, 2, 2, False, 5, 0, 0.0, False, 0.0, 0.0)
  if effort == 5:
    return EffortConfig(8, 2, 2, True, 8, 0, 0.0, False, 0.0, 0.0)
  if effort == 6:
    return EffortConfig(8, 2, 2, True, 8, 1, 0.05, False, 0.0, 0.0)
  if effort == 7:
    return EffortConfig(8, 2, 3, True, 10, 2, 0.08, False, 0.0, 0.0)
  if effort == 8:
    return EffortConfig(8, 3, 3, True, 14, 4, 0.12, False, 0.0, 0.0)
  if effort == 9:
    return EffortConfig(9, 3, 4, True, 16, 4, 0.12, True, 0.16, 20.0)
  return EffortConfig(10, 3, 5, True, 22, 6, 0.18, True, 0.24, 28.0)


def finish_block_candidates(block: bytes, spec: FormatSpec, effort: int, seeds: Sequence[Candidate]) -> list[Candidate]:
  cfg = effort_config(effort)
  weights = PERCEPTUAL_WEIGHTS if cfg.perceptual else RGB_WEIGHTS
  candidates: dict[tuple[int, int, bytes], Candidate] = {}

  for cand in seeds:
    add_candidate(candidates, cand)

    refined = refine_candidate(block, cand, spec, weights, cfg.refine_iters)
    add_candidate(candidates, refined)

    searched = local_search_candidate(block, refined, spec, weights, cfg.local_radius, cfg.local_passes)
    add_candidate(candidates, searched)

    extra = endpoint_variations(block, searched, spec, weights, effort, cfg.local_radius, max_extra=cfg.max_candidates * 4)
    for item in extra:
      add_candidate(candidates, item)

  result = sorted(candidates.values(), key=lambda c: c.error)
  if not result:
    black = evaluate_rgb_pair(block, (0, 0, 0), (0, 0, 0), spec, weights)
    result = [black]
  return result[:cfg.max_candidates]


def generate_block_candidates(block: bytes, spec: FormatSpec, effort: int) -> list[Candidate]:
  cfg = effort_config(effort)
  weights = PERCEPTUAL_WEIGHTS if cfg.perceptual else RGB_WEIGHTS
  seeds = [evaluate_rgb_pair(block, c0_rgb, c1_rgb, spec, weights) for c0_rgb, c1_rgb in initial_endpoint_pairs(block, effort, weights)]
  return finish_block_candidates(block, spec, effort, seeds)


def encode_chunk(items: list[tuple[int, bytes]], spec_key: str, effort: int) -> list[tuple[int, list[Candidate]]]:
  spec = FORMATS[spec_key]
  return [(idx, generate_block_candidates(block, spec, effort)) for idx, block in items]


def extract_blocks(rgb_bytes: bytes, width: int, height: int) -> list[bytes]:
  blocks_x = width // 4
  blocks_y = height // 4
  blocks: list[bytes] = []
  for by in range(blocks_y):
    for bx in range(blocks_x):
      block = bytearray()
      for yy in range(4):
        start = ((by * 4 + yy) * width + bx * 4) * 3
        block.extend(rgb_bytes[start:start + 12])
      blocks.append(bytes(block))
  return blocks


EDGE_LEFT_POS = (0, 1, 2, 12, 13, 14, 24, 25, 26, 36, 37, 38)
EDGE_RIGHT_POS = (9, 10, 11, 21, 22, 23, 33, 34, 35, 45, 46, 47)
EDGE_UP_POS = (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)
EDGE_DOWN_POS = (36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47)
EDGE_POSITIONS = (EDGE_LEFT_POS, EDGE_RIGHT_POS, EDGE_UP_POS, EDGE_DOWN_POS)
EDGE_REVERSE = (1, 0, 3, 2)


def select_profile_prefix(spec: FormatSpec | None, progress_label: str) -> str:
  if spec is not None:
    return spec.suffix
  parts = progress_label.strip().split()
  return parts[0] if parts else "select"


def build_select_neighbor_links(original_blocks: Sequence[bytes], blocks_x: int, blocks_y: int) -> list[list[tuple[int, int, int, tuple[int, ...]]]]:
  links: list[list[tuple[int, int, int, tuple[int, ...]]]] = []
  count = len(original_blocks)
  for idx in range(count):
    x = idx % blocks_x
    y = idx // blocks_x
    cur_links: list[tuple[int, int, int, tuple[int, ...]]] = []
    neighbors: list[tuple[int, int]] = []
    if x > 0:
      neighbors.append((idx - 1, 0))
    if x + 1 < blocks_x:
      neighbors.append((idx + 1, 1))
    if y > 0:
      neighbors.append((idx - blocks_x, 2))
    if y + 1 < blocks_y:
      neighbors.append((idx + blocks_x, 3))
    cur = original_blocks[idx]
    for nidx, direction in neighbors:
      reverse = EDGE_REVERSE[direction]
      cur_pos = EDGE_POSITIONS[direction]
      nei_pos = EDGE_POSITIONS[reverse]
      nei = original_blocks[nidx]
      orig_delta = tuple(int(cur[cur_pos[i]]) - int(nei[nei_pos[i]]) for i in range(12))
      cur_links.append((nidx, direction, reverse, orig_delta))
    links.append(cur_links)
  return links


def select_edge_delta_score(cand_decoded: bytes, neighbor_decoded: bytes, direction: int, reverse: int,
                            orig_delta: tuple[int, ...], weights12: tuple[float, ...]) -> float:
  pos_a = EDGE_POSITIONS[direction]
  pos_b = EDGE_POSITIONS[reverse]
  total = 0.0
  for i in range(12):
    d = int(cand_decoded[pos_a[i]]) - int(neighbor_decoded[pos_b[i]]) - orig_delta[i]
    total += weights12[i] * d * d
  return total


def select_seam_score_fast(index: int, cand_decoded: bytes, selected_decoded: Sequence[bytes],
                           links: Sequence[Sequence[tuple[int, int, int, tuple[int, ...]]]],
                           weights12: tuple[float, ...]) -> float:
  total = 0.0
  for nidx, direction, reverse, orig_delta in links[index]:
    total += select_edge_delta_score(cand_decoded, selected_decoded[nidx], direction, reverse, orig_delta, weights12)
  return total


def block_error_against_adjusted_rgb_fast(decoded: bytes, original: bytes, adjustment_rgb: Sequence[float],
                                          weights: tuple[float, float, float]) -> float:
  # The residual diffusion used here stores only one RGB adjustment per 4x4
  # block. The older code expanded the same RGB adjustment to all 16 pixels,
  # which was equivalent but much more expensive.
  total = 0.0
  ar = adjustment_rgb[0]
  ag = adjustment_rgb[1]
  ab = adjustment_rgb[2]
  wr, wg, wb = weights
  for pix in range(16):
    base = pix * 3
    target = clamp_float(float(original[base]) + ar, 0.0, 255.0)
    d = float(decoded[base]) - target
    total += wr * d * d
    target = clamp_float(float(original[base + 1]) + ag, 0.0, 255.0)
    d = float(decoded[base + 1]) - target
    total += wg * d * d
    target = clamp_float(float(original[base + 2]) + ab, 0.0, 255.0)
    d = float(decoded[base + 2]) - target
    total += wb * d * d
  return total


def select_auto_candidate_limits(effort: int, cfg: EffortConfig, options: SelectOptions | None) -> tuple[int, int]:
  seam_limit = cfg.max_candidates
  residual_limit = cfg.max_candidates
  if options is not None and options.seam_candidates > 0:
    seam_limit = options.seam_candidates
  elif cfg.seam_passes > 0:
    # Select is intentionally a visual/global stage. The independent encoder can
    # keep a wider retained pool, but seam selection does not need to test every
    # retained candidate on every pass. These defaults are speed-oriented and can
    # be overridden with --select-candidates.
    if effort >= 10:
      # e10 keeps a wider seam set, but still avoids testing all retained candidates.
      seam_limit = min(cfg.max_candidates, 6)
    elif effort >= 9:
      # e9 is dominated by select. 4 candidates/block keeps the global seam stage
      # useful while avoiding the very expensive full retained-pool sweep.
      seam_limit = min(cfg.max_candidates, 4)
    elif effort >= 8:
      seam_limit = min(cfg.max_candidates, 6)
    else:
      seam_limit = min(cfg.max_candidates, 6)

  if options is not None and options.residual_candidates > 0:
    residual_limit = options.residual_candidates
  elif cfg.residual:
    if effort >= 10:
      residual_limit = min(seam_limit, 2)
    elif effort >= 9:
      # Residual diffusion is now applied only on the active seam-neighborhood.
      # Testing one best local candidate per active block gives most of the speed
      # benefit and avoids the old residual bottleneck. Use
      # --select-residual-candidates to widen it when needed.
      residual_limit = min(seam_limit, 1)
    else:
      residual_limit = min(seam_limit, 4)
  else:
    residual_limit = seam_limit

  return max(1, seam_limit), max(1, residual_limit)


def select_auto_early_stop_threshold(block_count: int, options: SelectOptions | None) -> int:
  if options is not None and not options.early_stop:
    return -1
  if options is not None and options.early_stop_threshold > 0:
    return options.early_stop_threshold
  ratio = 96 if options is None else max(1, options.early_stop_ratio)
  return max(32, block_count // ratio)


def prune_select_candidate_lists(candidate_lists: Sequence[list[Candidate]], limit: int) -> tuple[list[list[Candidate]], int, int]:
  pruned: list[list[Candidate]] = []
  original_count = 0
  active_count = 0
  for cands in candidate_lists:
    original_count += len(cands)
    if len(cands) > limit:
      use = cands[:limit]
    else:
      use = cands
    active_count += len(use)
    pruned.append(use)
  return pruned, original_count, active_count


def diffuse_residual_fast(selected_indices: list[int], selected_decoded: list[bytes], candidate_lists: Sequence[list[Candidate]],
                          original_blocks: Sequence[bytes], blocks_x: int, blocks_y: int, cfg: EffortConfig,
                          weights: tuple[float, float, float], links: Sequence[Sequence[tuple[int, int, int, tuple[int, ...]]]],
                          weights12: tuple[float, ...], residual_limit: int, profile: ProfileMetrics | None, prefix: str,
                          active_indices: Sequence[int] | None = None) -> None:
  total_t0 = time.perf_counter()
  # The diffusion model applies the same RGB adjustment to every pixel in a
  # block. Keep it as 3 floats per block instead of an expanded 48-float vector.
  residual = [[0.0, 0.0, 0.0] for _ in range(len(selected_indices))]
  active_mask: list[bool] | None = None
  if active_indices is not None:
    active_mask = [False] * len(selected_indices)
    for idx in active_indices:
      if 0 <= idx < len(active_mask):
        active_mask[idx] = True

  def add_avg_residual(dst: int, avg: tuple[float, float, float], factor: float) -> None:
    vals = residual[dst]
    vals[0] = clamp_float(vals[0] + avg[0] * factor, -cfg.residual_clamp, cfg.residual_clamp)
    vals[1] = clamp_float(vals[1] + avg[1] * factor, -cfg.residual_clamp, cfg.residual_clamp)
    vals[2] = clamp_float(vals[2] + avg[2] * factor, -cfg.residual_clamp, cfg.residual_clamp)

  tested = 0
  changed = 0
  seam_links = 0
  block_count = len(selected_indices)
  for y in range(blocks_y):
    for x in range(blocks_x):
      idx = y * blocks_x + x
      best_idx = selected_indices[idx]
      candidates = candidate_lists[idx]
      idx_links = links[idx]
      if active_mask is None or active_mask[idx]:
        best_score = float("inf")
        limit = min(max(1, residual_limit), len(candidates))
        test_indices = list(range(limit))
        if selected_indices[idx] >= limit and selected_indices[idx] < len(candidates):
          test_indices.append(selected_indices[idx])
        for cand_idx in test_indices:
          cand = candidates[cand_idx]
          tested += 1
          decoded = cand.decoded
          score = block_error_against_adjusted_rgb_fast(decoded, original_blocks[idx], residual[idx], weights)
          if idx_links:
            seam_links += len(idx_links)
            score += cfg.seam_weight * select_seam_score_fast(idx, decoded, selected_decoded, links, weights12)
          if score < best_score:
            best_score = score
            best_idx = cand_idx
        if best_idx != selected_indices[idx]:
          changed += 1
          selected_indices[idx] = best_idx
          selected_decoded[idx] = candidates[best_idx].decoded

      best_decoded = selected_decoded[idx]
      avg = [0.0, 0.0, 0.0]
      orig = original_blocks[idx]
      for pix in range(16):
        base = pix * 3
        for ch in range(3):
          avg[ch] += float(orig[base + ch]) - float(best_decoded[base + ch])
      avg_tuple = tuple((avg[ch] / 16.0) * cfg.residual_strength for ch in range(3))

      if x + 1 < blocks_x:
        add_avg_residual(idx + 1, avg_tuple, 7.0 / 16.0)
      if y + 1 < blocks_y:
        if x > 0:
          add_avg_residual(idx + blocks_x - 1, avg_tuple, 3.0 / 16.0)
        add_avg_residual(idx + blocks_x, avg_tuple, 5.0 / 16.0)
        if x + 1 < blocks_x:
          add_avg_residual(idx + blocks_x + 1, avg_tuple, 1.0 / 16.0)

  if profile is not None:
    profile.add_time(f"{prefix}.select.residual.total", time.perf_counter() - total_t0)
    profile.add_count(f"{prefix}.select.residual.blocks", block_count)
    profile.add_count(f"{prefix}.select.residual.adjustment_channels", 3)
    profile.add_count(f"{prefix}.select.residual.candidates_tested", tested)
    profile.add_count(f"{prefix}.select.residual.seam_links", seam_links)
    profile.add_count(f"{prefix}.select.residual.changed", changed)
    if active_mask is not None:
      active_count = sum(1 for item in active_mask if item)
      profile.add_count(f"{prefix}.select.residual.active_blocks", active_count)
      profile.add_count(f"{prefix}.select.residual.skipped_blocks", block_count - active_count)
    else:
      profile.add_count(f"{prefix}.select.residual.active_blocks", block_count)
      profile.add_count(f"{prefix}.select.residual.skipped_blocks", 0)


def select_candidates(candidate_lists: Sequence[list[Candidate]], original_blocks: Sequence[bytes],
                      blocks_x: int, blocks_y: int, effort: int,
                      progress: Progress | None = None, progress_label: str = "select",
                      profile: ProfileMetrics | None = None, spec: FormatSpec | None = None,
                      select_options: SelectOptions | None = None) -> list[Candidate]:
  total_t0 = time.perf_counter()
  cfg = effort_config(effort)
  weights = PERCEPTUAL_WEIGHTS if cfg.perceptual else RGB_WEIGHTS
  weights12 = (weights[0], weights[1], weights[2]) * 4
  prefix = select_profile_prefix(spec, progress_label)
  block_count = len(candidate_lists)
  seam_limit, residual_limit = select_auto_candidate_limits(effort, cfg, select_options)
  early_stop_threshold = select_auto_early_stop_threshold(block_count, select_options)
  candidate_lists, original_candidate_count, active_candidate_count = prune_select_candidate_lists(candidate_lists, seam_limit)
  selected_indices = [0] * block_count
  selected_decoded: list[bytes] = [b""] * block_count

  profiled_nonoverlap = 0.0
  if profile is not None:
    profile.add_count(f"{prefix}.select.config.effort", effort)
    profile.add_count(f"{prefix}.select.config.seam_passes", cfg.seam_passes)
    profile.add_count(f"{prefix}.select.config.residual", 1 if cfg.residual else 0)
    profile.add_count(f"{prefix}.select.config.max_candidates", cfg.max_candidates)
    profile.add_count(f"{prefix}.select.config.seam_candidate_limit", seam_limit)
    profile.add_count(f"{prefix}.select.config.residual_candidate_limit", residual_limit)
    profile.add_count(f"{prefix}.select.config.residual_active", 1 if (select_options is None or select_options.residual_active) else 0)
    profile.add_count(f"{prefix}.select.config.early_stop", 1 if early_stop_threshold >= 0 else 0)
    profile.add_count(f"{prefix}.select.config.early_stop_threshold", max(0, early_stop_threshold))
    profile.add_count(f"{prefix}.select.blocks", block_count)
    profile.add_count(f"{prefix}.select.candidates_original", original_candidate_count)
    profile.add_count(f"{prefix}.select.candidates_active", active_candidate_count)
    profile.add_count(f"{prefix}.select.pruned_candidates", original_candidate_count - active_candidate_count)

  ensure_t0 = time.perf_counter()
  candidate_count = 0
  for idx, cands in enumerate(candidate_lists):
    candidate_count += len(cands)
    for cand in cands:
      if not cand.decoded:
        if spec is None:
          raise RuntimeError("select needs FormatSpec when candidates do not already contain decoded pixels")
        ensure_candidate_decoded(cand, spec)
    selected_decoded[idx] = cands[0].decoded
  ensure_dt = time.perf_counter() - ensure_t0
  profiled_nonoverlap += ensure_dt
  if profile is not None:
    profile.add_time(f"{prefix}.select.setup.ensure_decoded", ensure_dt)
    profile.add_count(f"{prefix}.select.candidates", candidate_count)

  links_t0 = time.perf_counter()
  links = build_select_neighbor_links(original_blocks, blocks_x, blocks_y)
  links_dt = time.perf_counter() - links_t0
  profiled_nonoverlap += links_dt
  if profile is not None:
    profile.add_time(f"{prefix}.select.setup.neighbor_links", links_dt)
    profile.add_count(f"{prefix}.select.neighbor_links", sum(len(item) for item in links))

  total_steps = cfg.seam_passes + (1 if cfg.residual else 0)
  done_steps = 0
  if progress is not None and total_steps > 0:
    progress.show(progress_label, 0, total_steps, force=True)

  seam_total_time = 0.0
  if cfg.seam_passes > 0:
    count = len(candidate_lists)
    active_indices: list[int] | None = None
    for pass_no in range(cfg.seam_passes):
      pass_t0 = time.perf_counter()
      if active_indices is None:
        active_block_count = count
        if pass_no % 2 == 0:
          order: Iterable[int] = range(count)
        else:
          order = reversed(range(count))
      else:
        active_block_count = len(active_indices)
        if active_block_count == 0:
          if profile is not None:
            profile.add_count(f"{prefix}.select.early_stop_pass", pass_no)
            profile.add_count(f"{prefix}.select.early_stop_changed", 0)
          break
        if pass_no % 2 == 0:
          order = active_indices
        else:
          order = reversed(active_indices)

      next_active = [False] * count
      candidates_tested = 0
      seam_links = 0
      changed = 0
      for idx in order:
        best_idx = selected_indices[idx]
        best_score = float("inf")
        idx_links = links[idx]
        cands = candidate_lists[idx]
        for cand_idx, cand in enumerate(cands):
          candidates_tested += 1
          score = cand.error
          if idx_links:
            seam_links += len(idx_links)
            score += cfg.seam_weight * select_seam_score_fast(idx, cand.decoded, selected_decoded, links, weights12)
          if score < best_score:
            best_score = score
            best_idx = cand_idx
        if best_idx != selected_indices[idx]:
          changed += 1
          selected_indices[idx] = best_idx
          selected_decoded[idx] = cands[best_idx].decoded
          next_active[idx] = True
          for nidx, _direction, _reverse, _orig_delta in idx_links:
            next_active[nidx] = True

      pass_dt = time.perf_counter() - pass_t0
      seam_total_time += pass_dt
      profiled_nonoverlap += pass_dt
      if profile is not None:
        profile.add_time(f"{prefix}.select.pass{pass_no}.total", pass_dt)
        profile.add_count(f"{prefix}.select.pass{pass_no}.blocks", active_block_count)
        profile.add_count(f"{prefix}.select.pass{pass_no}.candidates_tested", candidates_tested)
        profile.add_count(f"{prefix}.select.pass{pass_no}.seam_links", seam_links)
        profile.add_count(f"{prefix}.select.pass{pass_no}.changed", changed)

      done_steps += 1
      if progress is not None:
        progress.show(progress_label, done_steps, total_steps)

      active_indices = [idx for idx, is_active in enumerate(next_active) if is_active]
      if profile is not None:
        profile.add_count(f"{prefix}.select.pass{pass_no}.next_active", len(active_indices))

      if early_stop_threshold >= 0 and pass_no + 1 >= 2 and changed <= early_stop_threshold:
        if profile is not None:
          profile.add_count(f"{prefix}.select.early_stop_pass", pass_no)
          profile.add_count(f"{prefix}.select.early_stop_changed", changed)
        break
  elif profile is not None:
    profile.add_count(f"{prefix}.select.seam_skipped", 1)

  if profile is not None and cfg.seam_passes > 0:
    profile.add_time(f"{prefix}.select.seam_passes.total", seam_total_time)

  if cfg.residual:
    residual_t0 = time.perf_counter()
    residual_active_indices = active_indices if (select_options is None or select_options.residual_active) else None
    diffuse_residual_fast(selected_indices, selected_decoded, candidate_lists, original_blocks, blocks_x, blocks_y, cfg, weights, links, weights12, residual_limit, profile, prefix, residual_active_indices)
    residual_dt = time.perf_counter() - residual_t0
    profiled_nonoverlap += residual_dt
    done_steps += 1
    if progress is not None:
      progress.show(progress_label, done_steps, total_steps)
  elif profile is not None:
    profile.add_count(f"{prefix}.select.residual_skipped", 1)

  build_t0 = time.perf_counter()
  selected = [candidate_lists[idx][selected_indices[idx]] for idx in range(block_count)]
  build_dt = time.perf_counter() - build_t0
  profiled_nonoverlap += build_dt

  total_dt = time.perf_counter() - total_t0
  if profile is not None:
    profile.add_time(f"{prefix}.select.final_list_build", build_dt)
    profile.add_time(f"{prefix}.select.profiled_nonoverlap", profiled_nonoverlap)
    profile.add_time(f"{prefix}.select.total_internal", total_dt)
    profile.add_time(f"{prefix}.select.unaccounted", max(0.0, total_dt - profiled_nonoverlap))
    profile.add_count(f"{prefix}.select.final_blocks", block_count)

  return selected


def add_scored_pair(scored: list[tuple[float, int, int]], limit: int, error: float, c0: int, c1: int) -> None:
  # Small fixed top-K buffer for score-only GPU passes. Avoid sorting if the
  # incoming pair is already worse than the retained worst candidate.
  for i, (_, old_c0, old_c1) in enumerate(scored):
    if old_c0 == c0 and old_c1 == c1:
      if error < scored[i][0]:
        scored[i] = (error, c0, c1)
        scored.sort(key=lambda item: item[0])
      return

  if limit <= 0:
    return
  if len(scored) >= limit and error >= scored[-1][0]:
    return

  scored.append((error, c0, c1))
  scored.sort(key=lambda item: item[0])
  if len(scored) > limit:
    del scored[limit:]


def gpu_score_batch_chunked(evaluator: OpenCLBlockEvaluator, batch: RecordBatch,
                            spec: FormatSpec, weights: tuple[float, float, float],
                            batch_limit: int) -> list[float]:
  if len(batch) == 0:
    return []

  out: list[float] = []
  for start in range(0, len(batch), batch_limit):
    end = start + batch_limit
    out.extend(evaluator.evaluate_errors_arrays(
      batch.block_indices[start:end],
      batch.c0s[start:end],
      batch.c1s[start:end],
      spec,
      weights,
    ))
  return out


def gpu_evaluate_raw_batch_chunked(evaluator: OpenCLBlockEvaluator, batch: RecordBatch,
                                   spec: FormatSpec, weights: tuple[float, float, float],
                                   batch_limit: int, include_decoded: bool = True) -> tuple[list[int], list[int], list[int], list[bytes], list[float], list[bytes]]:
  if len(batch) == 0:
    return [], [], [], [], [], []

  out_block_indices: list[int] = []
  out_c0s: list[int] = []
  out_c1s: list[int] = []
  out_selectors: list[bytes] = []
  out_errors: list[float] = []
  out_decoded: list[bytes] = []
  for start in range(0, len(batch), batch_limit):
    end = start + batch_limit
    block_indices, c0s, c1s, selectors, errors, decoded = evaluator.evaluate_records_raw_arrays(
      batch.block_indices[start:end],
      batch.c0s[start:end],
      batch.c1s[start:end],
      spec,
      weights,
      include_decoded,
    )
    out_block_indices.extend(block_indices)
    out_c0s.extend(c0s)
    out_c1s.extend(c1s)
    out_selectors.extend(selectors)
    out_errors.extend(errors)
    out_decoded.extend(decoded)
  return out_block_indices, out_c0s, out_c1s, out_selectors, out_errors, out_decoded


def finish_block_candidates_gpu(blocks: Sequence[bytes], spec: FormatSpec, effort: int,
                                seed_pool: RawRetainedPool, seed_work: RawWorkList,
                                evaluator: OpenCLBlockEvaluator,
                                gpu_batch: int, progress: Progress | None = None,
                                profile: ProfileMetrics | None = None) -> list[list[Candidate]]:
  profile_t0 = time.perf_counter()
  cfg = effort_config(effort)
  weights = PERCEPTUAL_WEIGHTS if cfg.perceptual else RGB_WEIGHTS
  store_limit = max(1, cfg.max_candidates)
  include_decoded = cfg.seam_passes > 0 or cfg.residual
  pool = seed_pool
  work_items = seed_work
  batch_limit = gpu_batch

  total_steps = cfg.refine_iters
  if cfg.local_radius > 0 and cfg.local_passes > 0:
    total_steps += 1
  if cfg.local_radius > 0 and cfg.max_candidates > 0:
    total_steps += 2

  done_steps = 0
  label = f"{spec.suffix} hybrid refine"
  if progress is not None and total_steps > 0:
    progress.show(label, 0, total_steps, force=True)

  profile_prefix = f"{spec.suffix}.hybrid_refine"
  profile_start_times = dict(profile.times) if profile is not None and profile.enabled else {}

  def profile_stage_delta(name: str) -> float:
    if profile is None or not profile.enabled:
      return 0.0
    return profile.times.get(name, 0.0) - profile_start_times.get(name, 0.0)

  def profile_count(name: str, value: int) -> None:
    if profile is not None:
      profile.add_count(name, value)

  def profile_time(name: str, seconds: float) -> None:
    if profile is not None:
      profile.add_time(name, seconds)

  if profile is not None:
    profile_count(f"{profile_prefix}.config.effort", effort)
    profile_count(f"{profile_prefix}.config.refine_iters", cfg.refine_iters)
    profile_count(f"{profile_prefix}.config.local_radius", cfg.local_radius)
    profile_count(f"{profile_prefix}.config.local_passes", cfg.local_passes)
    profile_count(f"{profile_prefix}.config.max_candidates", cfg.max_candidates)
    profile_count(f"{profile_prefix}.config.seam_passes", cfg.seam_passes)
    profile_count(f"{profile_prefix}.config.residual", 1 if cfg.residual else 0)
    profile_count(f"{profile_prefix}.initial_work_items", len(work_items))
    profile_count(f"{profile_prefix}.total_steps", total_steps)
    if cfg.refine_iters == 0:
      profile_count(f"{profile_prefix}.skipped_refine_stage", 1)
    if not (cfg.local_radius > 0 and cfg.local_passes > 0):
      profile_count(f"{profile_prefix}.skipped_local_stage", 1)
    if not (cfg.local_radius > 0 and cfg.max_candidates > 0):
      profile_count(f"{profile_prefix}.skipped_variations_stage", 1)

  def scored_pair_can_enter_pool(block_idx: int, error: float) -> bool:
    return pool.can_enter(block_idx, error)

  def decode_raw(records: RecordBatch, profile_key: str) -> tuple[list[int], list[int], list[int], list[bytes], list[float], list[bytes]]:
    decode_t0 = time.perf_counter()
    decoded = gpu_evaluate_raw_batch_chunked(evaluator, records, spec, weights, batch_limit, include_decoded)
    if profile is not None:
      profile.add_time(profile_key, time.perf_counter() - decode_t0)
    return decoded

  def rebuild_work_items_from_pool(profile_name: str) -> RawWorkList:
    return pool.to_work_list(profile, f"{spec.suffix}.{profile_name}.retained_rebuild")

  for pass_idx in range(cfg.refine_iters):
    pass_t0 = time.perf_counter()
    pass_active_total = 0
    pass_decode_total = 0
    pass_inserted_total = 0
    pass_rejected_total = 0
    pass_nochange_total = 0
    pass_inactive_total = 0
    active_seen = False
    profile_count(f"{profile_prefix}.pass_count", 1)
    profile_count(f"{profile_prefix}.pass{pass_idx}.start_work_items", len(work_items))
    for start in range(0, len(work_items), batch_limit):
      build_t0 = time.perf_counter()
      end = min(start + batch_limit, len(work_items))
      positions = [i for i in range(start, end) if work_items.active[i]]
      if not positions:
        pass_inactive_total += end - start
        if profile is not None:
          profile.add_time(f"{spec.suffix}.hybrid_refine.build_records", time.perf_counter() - build_t0)
        continue

      active_seen = True
      pass_active_total += len(positions)
      block_indices = [work_items.block_indices[i] for i in positions]
      selectors = [work_items.selectors[i] for i in positions]
      c0s = [work_items.c0s[i] for i in positions]
      c1s = [work_items.c1s[i] for i in positions]
      current_errors = [work_items.errors[i] for i in positions]
      if profile is not None:
        profile.add_time(f"{spec.suffix}.hybrid_refine.build_records", time.perf_counter() - build_t0)
        profile.add_count(f"{spec.suffix}.hybrid_refine.seed_count", len(positions))

      score_t0 = time.perf_counter()
      best_c0s, best_c1s, best_errors = evaluator.refine_fit_best_arrays(
        block_indices,
        selectors,
        c0s,
        c1s,
        current_errors,
        spec,
        weights,
      )
      if profile is not None:
        profile.add_time(f"{spec.suffix}.hybrid_refine.gpu_fit_stage", time.perf_counter() - score_t0)
        profile.add_count(f"{spec.suffix}.hybrid_refine.score_pairs", len(positions))

      filter_t0 = time.perf_counter()
      decode_positions: list[int] = []
      decode_records_batch = RecordBatch()
      for pos, block_idx, c0, c1, error in zip(positions, block_indices, best_c0s, best_c1s, best_errors):
        old_c0 = work_items.c0s[pos]
        old_c1 = work_items.c1s[pos]
        old_error = work_items.errors[pos]
        if c0 == old_c0 and c1 == old_c1:
          work_items.active[pos] = False
          pass_nochange_total += 1
        elif error <= old_error + 1e-9 and scored_pair_can_enter_pool(block_idx, error):
          decode_positions.append(pos)
          decode_records_batch.append(block_idx, c0, c1)
        else:
          work_items.active[pos] = False
          pass_rejected_total += 1
          if profile is not None and error <= old_error + 1e-9:
            profile.add_count(f"{spec.suffix}.hybrid_refine.skipped_decode_pool", 1)
      if profile is not None:
        profile.add_time(f"{spec.suffix}.hybrid_refine.filter_stage", time.perf_counter() - filter_t0)

      if len(decode_records_batch) == 0:
        continue

      pass_decode_total += len(decode_records_batch)
      if profile is not None:
        profile.add_count(f"{spec.suffix}.hybrid_refine.decode_pairs", len(decode_records_batch))
      merge_t0 = time.perf_counter()
      raw = decode_raw(decode_records_batch, f"{spec.suffix}.hybrid_refine.decode_stage")
      raw_block_indices, raw_c0s, raw_c1s, raw_selectors, raw_errors, raw_decoded = raw
      for pos, block_idx, c0, c1, selectors, error, decoded in zip(
        decode_positions, raw_block_indices, raw_c0s, raw_c1s, raw_selectors, raw_errors, raw_decoded
      ):
        old_c0 = work_items.c0s[pos]
        old_c1 = work_items.c1s[pos]
        old_selectors = work_items.selectors[pos]
        if pool.insert(block_idx, c0, c1, selectors, error, decoded):
          pass_inserted_total += 1
        else:
          pass_rejected_total += 1
        if c0 == old_c0 and c1 == old_c1 and selectors == old_selectors:
          work_items.active[pos] = False
        else:
          work_items.c0s[pos] = c0
          work_items.c1s[pos] = c1
          work_items.selectors[pos] = selectors
          work_items.errors[pos] = error
      if profile is not None:
        profile.add_time(f"{spec.suffix}.hybrid_refine.merge_stage", time.perf_counter() - merge_t0)

    stop_refine = not active_seen or not any(work_items.active)
    if stop_refine:
      profile_count(f"{profile_prefix}.stopped_no_active", 1)

    profile_count(f"{profile_prefix}.pass{pass_idx}.active_items", pass_active_total)
    profile_count(f"{profile_prefix}.pass{pass_idx}.inactive_items", pass_inactive_total)
    profile_count(f"{profile_prefix}.pass{pass_idx}.decode_pairs", pass_decode_total)
    profile_count(f"{profile_prefix}.pass{pass_idx}.pool_inserted", pass_inserted_total)
    profile_count(f"{profile_prefix}.pass{pass_idx}.rejected_or_deactivated", pass_rejected_total)
    profile_count(f"{profile_prefix}.pass{pass_idx}.nochange", pass_nochange_total)
    profile_time(f"{profile_prefix}.pass{pass_idx}.total", time.perf_counter() - pass_t0)

    done_steps += 1
    if progress is not None and total_steps > 0:
      progress.show(label, done_steps, total_steps)

    if stop_refine:
      done_steps = cfg.refine_iters
      if progress is not None and total_steps > 0:
        progress.show(label, done_steps, total_steps)
      break

  work_items = rebuild_work_items_from_pool("hybrid_refine")

  if cfg.local_radius > 0 and cfg.local_passes > 0:
    local_t0 = time.perf_counter()
    improved_total = 0

    for start in range(0, len(work_items), batch_limit):
      build_t0 = time.perf_counter()
      end = min(start + batch_limit, len(work_items))
      positions = list(range(start, end))
      block_indices = [work_items.block_indices[i] for i in positions]
      c0s = [work_items.c0s[i] for i in positions]
      c1s = [work_items.c1s[i] for i in positions]
      current_errors = [work_items.errors[i] for i in positions]
      if profile is not None:
        profile.add_time(f"{spec.suffix}.hybrid_local.build_records", time.perf_counter() - build_t0)
        profile.add_count(f"{spec.suffix}.hybrid_local.seed_count", len(positions))
        profile.add_count(f"{spec.suffix}.hybrid_local.generated_on_gpu_pairs.max", len(positions) * cfg.local_passes * 6 * cfg.local_radius * 2)

      score_t0 = time.perf_counter()
      best_c0s, best_c1s, best_errors = evaluator.local_search_best_arrays(
        block_indices,
        c0s,
        c1s,
        current_errors,
        spec,
        weights,
        cfg.local_radius,
        cfg.local_passes,
      )
      if profile is not None:
        profile.add_time(f"{spec.suffix}.hybrid_local.gpu_best_stage", time.perf_counter() - score_t0)

      filter_t0 = time.perf_counter()
      decode_positions: list[int] = []
      decode_records_batch = RecordBatch()
      for pos, block_idx, c0, c1, error in zip(positions, block_indices, best_c0s, best_c1s, best_errors):
        if error + 1e-9 < work_items.errors[pos]:
          if scored_pair_can_enter_pool(block_idx, error):
            decode_positions.append(pos)
            decode_records_batch.append(block_idx, c0, c1)
          elif profile is not None:
            profile.add_count(f"{spec.suffix}.hybrid_local.skipped_decode_pool", 1)
      if profile is not None:
        profile.add_time(f"{spec.suffix}.hybrid_local.filter_stage", time.perf_counter() - filter_t0)
        profile.add_count(f"{spec.suffix}.hybrid_local.returned_best", len(decode_records_batch))

      if len(decode_records_batch) == 0:
        continue

      if profile is not None:
        profile.add_count(f"{spec.suffix}.hybrid_local.decode_pairs", len(decode_records_batch))
      merge_t0 = time.perf_counter()
      raw = decode_raw(decode_records_batch, f"{spec.suffix}.hybrid_local.decode_stage")
      raw_block_indices, raw_c0s, raw_c1s, raw_selectors, raw_errors, raw_decoded = raw
      for pos, block_idx, c0, c1, selectors, error, decoded in zip(
        decode_positions, raw_block_indices, raw_c0s, raw_c1s, raw_selectors, raw_errors, raw_decoded
      ):
        if error + 1e-9 < work_items.errors[pos]:
          if pool.insert(block_idx, c0, c1, selectors, error, decoded):
            profile_count(f"{spec.suffix}.hybrid_local.pool_inserted", 1)
          else:
            profile_count(f"{spec.suffix}.hybrid_local.pool_rejected_after_decode", 1)
          work_items.c0s[pos] = c0
          work_items.c1s[pos] = c1
          work_items.selectors[pos] = selectors
          work_items.errors[pos] = error
          improved_total += 1
      if profile is not None:
        profile.add_time(f"{spec.suffix}.hybrid_local.merge_stage", time.perf_counter() - merge_t0)

    if profile is not None:
      profile.add_time(f"{spec.suffix}.hybrid_local.total", time.perf_counter() - local_t0)
      profile.add_count(f"{spec.suffix}.hybrid_local.improved", improved_total)

    done_steps += 1
    if progress is not None and total_steps > 0:
      progress.show(label, done_steps, total_steps)

    work_items = rebuild_work_items_from_pool("hybrid_local")

  if cfg.local_radius > 0 and cfg.max_candidates > 0:
    variation_t0 = time.perf_counter()
    keep_per_block = max(1, cfg.max_candidates)
    max_extra = cfg.max_candidates * 3
    scored_variations: list[list[tuple[float, int, int]]] = [[] for _ in blocks]
    returned_total = 0
    improved_total = 0

    for start in range(0, len(work_items), batch_limit):
      build_t0 = time.perf_counter()
      end = min(start + batch_limit, len(work_items))
      positions = list(range(start, end))
      block_indices = [work_items.block_indices[i] for i in positions]
      c0s = [work_items.c0s[i] for i in positions]
      c1s = [work_items.c1s[i] for i in positions]
      current_errors = [work_items.errors[i] for i in positions]
      if profile is not None:
        profile.add_time(f"{spec.suffix}.hybrid_variations.build_records", time.perf_counter() - build_t0)
        profile.add_count(f"{spec.suffix}.hybrid_variations.seed_count", len(positions))
        profile.add_count(f"{spec.suffix}.hybrid_variations.generated_on_gpu_pairs.max", len(positions) * max_extra)

      score_t0 = time.perf_counter()
      best_c0s, best_c1s, best_errors = evaluator.variation_search_best_arrays(
        block_indices,
        c0s,
        c1s,
        current_errors,
        spec,
        weights,
        cfg.local_radius,
        effort,
        max_extra,
      )
      if profile is not None:
        profile.add_time(f"{spec.suffix}.hybrid_variations.gpu_best_stage", time.perf_counter() - score_t0)

      topk_t0 = time.perf_counter()
      for pos, block_idx, c0, c1, error in zip(positions, block_indices, best_c0s, best_c1s, best_errors):
        if c0 == work_items.c0s[pos] and c1 == work_items.c1s[pos]:
          continue
        returned_total += 1
        if error + 1e-9 < work_items.errors[pos]:
          if scored_pair_can_enter_pool(block_idx, error):
            improved_total += 1
            add_scored_pair(scored_variations[block_idx], keep_per_block, error, c0, c1)
          elif profile is not None:
            profile.add_count(f"{spec.suffix}.hybrid_variations.skipped_decode_pool", 1)
        elif profile is not None:
          profile.add_count(f"{spec.suffix}.hybrid_variations.skipped_decode_not_improved", 1)
      if profile is not None:
        profile.add_time(f"{spec.suffix}.hybrid_variations.topk_stage", time.perf_counter() - topk_t0)

    done_steps += 1
    if progress is not None and total_steps > 0:
      progress.show(label, done_steps, total_steps)

    decode_t0 = time.perf_counter()
    decode_records_batch = RecordBatch()
    for block_idx, scored in enumerate(scored_variations):
      for _, c0, c1 in scored:
        decode_records_batch.append(block_idx, c0, c1)

    if profile is not None:
      profile.add_time(f"{spec.suffix}.hybrid_variations.decode_build", time.perf_counter() - decode_t0)
      profile.add_count(f"{spec.suffix}.hybrid_variations.decode_pairs", len(decode_records_batch))
      profile.add_count(f"{spec.suffix}.hybrid_variations.returned_best", returned_total)
      profile.add_count(f"{spec.suffix}.hybrid_variations.improved", improved_total)
    merge_t0 = time.perf_counter()
    raw = decode_raw(decode_records_batch, f"{spec.suffix}.hybrid_variations.decode_stage")
    raw_block_indices, raw_c0s, raw_c1s, raw_selectors, raw_errors, raw_decoded = raw
    variation_inserted = 0
    variation_rejected = 0
    for block_idx, c0, c1, selectors, error, decoded in zip(raw_block_indices, raw_c0s, raw_c1s, raw_selectors, raw_errors, raw_decoded):
      if pool.insert(block_idx, c0, c1, selectors, error, decoded):
        variation_inserted += 1
      else:
        variation_rejected += 1
    profile_count(f"{spec.suffix}.hybrid_variations.pool_inserted", variation_inserted)
    profile_count(f"{spec.suffix}.hybrid_variations.pool_rejected_after_decode", variation_rejected)
    if profile is not None:
      profile.add_time(f"{spec.suffix}.hybrid_variations.merge_stage", time.perf_counter() - merge_t0)
      profile.add_time(f"{spec.suffix}.hybrid_variations.total", time.perf_counter() - variation_t0)

    done_steps += 1
    if progress is not None and total_steps > 0:
      progress.show(label, total_steps, total_steps, force=True)

  final_t0 = time.perf_counter()
  result = pool.to_candidate_lists(blocks, spec, weights, profile)
  final_time = time.perf_counter() - final_t0
  if profile is not None:
    profile.add_time(f"{spec.suffix}.hybrid_refine.final_candidate_build", final_time)
    profile.add_count(f"{spec.suffix}.hybrid_refine.final_candidates", sum(len(items) for items in result))
    profile.add_count(f"{spec.suffix}.hybrid_refine.final_blocks", len(result))
    total_time = time.perf_counter() - profile_t0
    profile.add_time(f"{spec.suffix}.hybrid_refine.total", total_time)
    tracked_keys = [
      f"{spec.suffix}.hybrid_refine.build_records",
      f"{spec.suffix}.hybrid_refine.gpu_fit_stage",
      f"{spec.suffix}.hybrid_refine.filter_stage",
      f"{spec.suffix}.hybrid_refine.decode_stage",
      f"{spec.suffix}.hybrid_refine.merge_stage",
      f"{spec.suffix}.hybrid_refine.retained_rebuild",
      f"{spec.suffix}.hybrid_refine.final_candidate_build",
      f"{spec.suffix}.hybrid_local.total",
      f"{spec.suffix}.hybrid_variations.total",
    ]
    tracked_total = sum(profile_stage_delta(name) for name in tracked_keys)
    for pass_idx in range(cfg.refine_iters):
      tracked_total += profile_stage_delta(f"{profile_prefix}.pass{pass_idx}.total")
    # pass*.total intentionally overlaps the finer refine-stage timings.  Keep both
    # forms visible, but compute a second non-overlapping estimate for hidden time.
    non_overlap_keys = [
      f"{spec.suffix}.hybrid_refine.build_records",
      f"{spec.suffix}.hybrid_refine.gpu_fit_stage",
      f"{spec.suffix}.hybrid_refine.filter_stage",
      f"{spec.suffix}.hybrid_refine.decode_stage",
      f"{spec.suffix}.hybrid_refine.merge_stage",
      f"{spec.suffix}.hybrid_refine.retained_rebuild",
      f"{spec.suffix}.hybrid_refine.final_candidate_build",
      f"{spec.suffix}.hybrid_local.total",
      f"{spec.suffix}.hybrid_variations.total",
    ]
    non_overlap_total = sum(profile_stage_delta(name) for name in non_overlap_keys)
    profile.add_time(f"{spec.suffix}.hybrid_refine.profiled_nonoverlap", non_overlap_total)
    profile.add_time(f"{spec.suffix}.hybrid_refine.unaccounted", max(0.0, total_time - non_overlap_total))
    profile.add_time(f"{spec.suffix}.hybrid_refine.pass_total_overlap", tracked_total - non_overlap_total)
  return result


def encode_image_blocks_gpu(blocks: list[bytes], blocks_x: int, blocks_y: int, spec_key: str,
                            effort: int, gpu_batch: int, progress: Progress | None = None,
                            profile: ProfileMetrics | None = None,
                            initial_pairs_cache: Sequence[list[tuple[int, int]]] | None = None,
                            evaluator: OpenCLBlockEvaluator | None = None,
                            select_options: SelectOptions | None = None) -> EncodedImage:
  profile_t0 = time.perf_counter()
  spec = FORMATS[spec_key]
  cfg = effort_config(effort)
  weights = PERCEPTUAL_WEIGHTS if cfg.perceptual else RGB_WEIGHTS
  include_decoded = cfg.seam_passes > 0 or cfg.residual
  progress_label = f"{spec.suffix} gpu encode"
  if evaluator is None:
    evaluator = OpenCLBlockEvaluator(blocks, profile)
    print(f"{spec.suffix} OpenCL device: {evaluator.device_name}", file=sys.stderr, flush=True)

  batch_records = RecordBatch()
  batch_limit = gpu_batch
  done_blocks = 0
  keep_per_block = max(1, cfg.max_candidates)
  scored_initial: list[list[tuple[float, int, int]]] = [[] for _ in blocks]

  if progress is not None:
    progress.show(progress_label, 0, len(blocks), force=True)

  def flush_score_records() -> None:
    if len(batch_records) == 0:
      return
    if profile is not None:
      profile.add_count(f"{spec.suffix}.initial.score_pairs", len(batch_records))
    score_t0 = time.perf_counter()
    errors = gpu_score_batch_chunked(evaluator, batch_records, spec, weights, batch_limit)
    if profile is not None:
      profile.add_time(f"{spec.suffix}.initial.score_stage", time.perf_counter() - score_t0)
    topk_t0 = time.perf_counter()
    for block_idx, c0, c1, error in zip(batch_records.block_indices, batch_records.c0s, batch_records.c1s, errors):
      add_scored_pair(scored_initial[block_idx], keep_per_block, error, c0, c1)
    if profile is not None:
      profile.add_time(f"{spec.suffix}.initial.topk_stage", time.perf_counter() - topk_t0)
    batch_records.clear()

  build_t0 = time.perf_counter()
  local_endpoint_cache: dict[bytes, list[tuple[int, int]]] = {}
  for idx, block in enumerate(blocks):
    if initial_pairs_cache is not None:
      pairs_565 = initial_pairs_cache[idx]
    else:
      pairs_565 = local_endpoint_cache.get(block)
      if pairs_565 is None:
        pairs_565 = initial_endpoint_pairs_565(block, effort, weights)
        local_endpoint_cache[block] = pairs_565
        if profile is not None:
          profile.add_count(f"{spec.suffix}.initial.unique_blocks", 1)
      else:
        if profile is not None:
          profile.add_count(f"{spec.suffix}.initial.cache_hits", 1)

    for c0_565, c1_565 in pairs_565:
      batch_records.append(idx, c0_565, c1_565)
      if profile is not None:
        profile.add_count(f"{spec.suffix}.initial.generated_pairs", 1)
      if len(batch_records) >= batch_limit:
        if profile is not None:
          profile.add_time(f"{spec.suffix}.initial.build_records", time.perf_counter() - build_t0)
        flush_score_records()
        build_t0 = time.perf_counter()
    done_blocks += 1
    if progress is not None:
      progress.show(progress_label, done_blocks, len(blocks))
  if profile is not None:
    profile.add_time(f"{spec.suffix}.initial.build_records", time.perf_counter() - build_t0)
  flush_score_records()

  decode_build_t0 = time.perf_counter()
  decode_records_batch = RecordBatch()
  for block_idx, scored in enumerate(scored_initial):
    for _, c0, c1 in scored:
      decode_records_batch.append(block_idx, c0, c1)

  if profile is not None:
    profile.add_time(f"{spec.suffix}.initial.decode_build", time.perf_counter() - decode_build_t0)
    profile.add_count(f"{spec.suffix}.initial.decode_pairs", len(decode_records_batch))
  decode_t0 = time.perf_counter()
  raw = gpu_evaluate_raw_batch_chunked(evaluator, decode_records_batch, spec, weights, batch_limit, include_decoded)
  if profile is not None:
    profile.add_time(f"{spec.suffix}.initial.decode_stage", time.perf_counter() - decode_t0)

  seed_pool = RawRetainedPool(len(blocks), keep_per_block, profile)
  seed_work = RawWorkList()
  raw_block_indices, raw_c0s, raw_c1s, raw_selectors, raw_errors, raw_decoded = raw
  for block_idx, c0, c1, selectors, error, decoded in zip(raw_block_indices, raw_c0s, raw_c1s, raw_selectors, raw_errors, raw_decoded):
    inserted = seed_pool.insert(block_idx, c0, c1, selectors, error, decoded)
    if inserted:
      seed_work.append(block_idx, c0, c1, selectors, error, True)

  candidate_lists = finish_block_candidates_gpu(blocks, spec, effort, seed_pool, seed_work, evaluator, gpu_batch, progress, profile)

  select_t0 = time.perf_counter()
  selected = select_candidates(
    candidate_lists,
    blocks,
    blocks_x,
    blocks_y,
    effort,
    progress,
    f"{spec.suffix} select",
    profile,
    spec,
    select_options,
  )
  if profile is not None:
    profile.add_time(f"{spec.suffix}.select", time.perf_counter() - select_t0)
    profile.add_time(f"{spec.suffix}.gpu_encode.total", time.perf_counter() - profile_t0)
  return EncodedImage(
    spec=spec,
    width=blocks_x * 4,
    height=blocks_y * 4,
    blocks_x=blocks_x,
    blocks_y=blocks_y,
    blocks=selected,
  )


def encode_image_blocks(blocks: list[bytes], blocks_x: int, blocks_y: int, spec_key: str,
                        effort: int, jobs: int, progress: Progress | None = None,
                        use_gpu: bool = False, gpu_batch: int = 262144,
                        profile: ProfileMetrics | None = None,
                        initial_pairs_cache: Sequence[list[tuple[int, int]]] | None = None,
                        gpu_evaluator: OpenCLBlockEvaluator | None = None,
                        select_options: SelectOptions | None = None) -> EncodedImage:
  if use_gpu:
    return encode_image_blocks_gpu(blocks, blocks_x, blocks_y, spec_key, effort, gpu_batch, progress, profile, initial_pairs_cache, gpu_evaluator, select_options)

  profile_t0 = time.perf_counter()
  spec = FORMATS[spec_key]
  indexed_blocks = list(enumerate(blocks))
  candidate_lists: list[list[Candidate] | None] = [None] * len(blocks)
  progress_label = f"{spec.suffix} encode"

  if jobs == 0:
    workers = os.cpu_count() or 1
  else:
    workers = jobs

  if progress is not None:
    progress.show(progress_label, 0, len(blocks), force=True)

  done = 0
  if workers <= 1 or len(blocks) <= 1:
    for idx, block in indexed_blocks:
      candidate_lists[idx] = generate_block_candidates(block, spec, effort)
      done += 1
      if progress is not None:
        progress.show(progress_label, done, len(blocks))
  else:
    chunk_size = max(1, math.ceil(len(indexed_blocks) / (workers * 4)))
    chunks = [indexed_blocks[i:i + chunk_size] for i in range(0, len(indexed_blocks), chunk_size)]
    with concurrent.futures.ProcessPoolExecutor(max_workers=workers) as executor:
      futures = [executor.submit(encode_chunk, chunk, spec_key, effort) for chunk in chunks]
      for future in concurrent.futures.as_completed(futures):
        result = future.result()
        for idx, cands in result:
          candidate_lists[idx] = cands
        done += len(result)
        if progress is not None:
          progress.show(progress_label, done, len(blocks))

  if any(cands is None for cands in candidate_lists):
    raise RuntimeError("internal error: not all blocks were encoded")

  typed_candidate_lists = [cands for cands in candidate_lists if cands is not None]
  selected = select_candidates(
    typed_candidate_lists,
    blocks,
    blocks_x,
    blocks_y,
    effort,
    progress,
    f"{spec.suffix} select",
    profile,
    spec,
    select_options,
  )
  if profile is not None:
    profile.add_time(f"{spec.suffix}.cpu_encode.total", time.perf_counter() - profile_t0)
  return EncodedImage(
    spec=spec,
    width=blocks_x * 4,
    height=blocks_y * 4,
    blocks_x=blocks_x,
    blocks_y=blocks_y,
    blocks=selected,
  )


def make_color_layer(blocks: Sequence[Candidate], which: str) -> bytes:
  out = bytearray()
  for block in blocks:
    value = block.c0 if which == "c0" else block.c1
    out.extend(struct.pack("<H", value))
  return bytes(out)


def ft_l2_selector_to_mask_code(sel: int) -> int:
  # Encoder selector order follows DXT interpolation:
  #   0=c0, 1=c1, 2=1/3 c1, 3=2/3 c1.
  # FT_L2 mask codes are linear alpha: 0=0, 1=85, 2=170, 3=255.
  return (0, 3, 1, 2)[sel & 0x03]


def make_mask_layer(encoded: EncodedImage) -> bytes:
  width = encoded.width
  height = encoded.height
  blocks_x = encoded.blocks_x
  spec = encoded.spec

  if spec.selector_bits == 2:
    stride = width // 4
    out = bytearray(stride * height)
    for y in range(height):
      row_off = y * stride
      for x_byte in range(stride):
        value = 0
        for p in range(4):
          x = x_byte * 4 + p
          bx = x // 4
          by = y // 4
          px = x & 3
          py = y & 3
          block = encoded.blocks[by * blocks_x + bx]
          sel = ft_l2_selector_to_mask_code(block.selectors[py * 4 + px])
          shift = 6 - p * 2
          value |= sel << shift
        out[row_off + x_byte] = value
    return bytes(out)

  stride = (width + 1) // 2
  out = bytearray(stride * height)
  for y in range(height):
    row_off = y * stride
    for x_byte in range(stride):
      value = 0
      for p in range(2):
        x = x_byte * 2 + p
        bx = x // 4
        by = y // 4
        px = x & 3
        py = y & 3
        block = encoded.blocks[by * blocks_x + bx]
        sel = block.selectors[py * 4 + px] & 0x0F
        if p == 0:
          value |= sel << 4
        else:
          value |= sel
      out[row_off + x_byte] = value
  return bytes(out)


def make_raw(encoded: EncodedImage) -> bytes:
  c0 = make_color_layer(encoded.blocks, "c0")
  c1 = make_color_layer(encoded.blocks, "c1")
  mask = make_mask_layer(encoded)
  return c0 + c1 + mask


def make_dxt(encoded: EncodedImage) -> bytes:
  spec = encoded.spec
  block_count = len(encoded.blocks)
  out = bytearray()
  out.extend(spec.magic)
  out.extend(struct.pack("<III", encoded.width, encoded.height, block_count))
  for block in encoded.blocks:
    out.extend(struct.pack("<HH", block.c0, block.c1))
    out.extend(ensure_packed_selectors(block, spec))
  return bytes(out)


def make_dxp(encoded: EncodedImage, raw: bytes, zlib_level: int) -> bytes:
  compressed = zlib.compress(raw, zlib_level)
  type_id = DXP_TYPE_ZLIB_L4 if encoded.spec.selector_bits == 4 else DXP_TYPE_ZLIB_L2
  header = struct.pack(
    "<3sBHH",
    b"DXP",
    type_id,
    encoded.width,
    encoded.height,
  )
  return header + compressed


def make_preview(encoded: EncodedImage) -> Image.Image:
  out = Image.new("RGB", (encoded.width, encoded.height))
  pixels = out.load()
  for by in range(encoded.blocks_y):
    for bx in range(encoded.blocks_x):
      block = encoded.blocks[by * encoded.blocks_x + bx]
      decoded = ensure_candidate_decoded(block, encoded.spec)
      for py in range(4):
        for px in range(4):
          i = (py * 4 + px) * 3
          pixels[bx * 4 + px, by * 4 + py] = (decoded[i], decoded[i + 1], decoded[i + 2])
  return out


def macro_prefix(base: str, suffix: str) -> str:
  stem = Path(base).name
  stem = re.sub(r"[^A-Za-z0-9]+", "_", stem).strip("_")
  if not stem:
    stem = "FT812_DXT"
  return f"{stem}_{suffix}".upper()


def header_text(encoded: EncodedImage, base: str) -> str:
  spec = encoded.spec
  prefix = macro_prefix(base, spec.suffix)
  block_count = len(encoded.blocks)
  c0_offset = 0
  c0_size = block_count * 2
  c1_offset = c0_offset + c0_size
  c1_size = c0_size
  mask_offset = c1_offset + c1_size
  if spec.selector_bits == 2:
    mask_stride = encoded.width // 4
  else:
    mask_stride = (encoded.width + 1) // 2
  mask_size = mask_stride * encoded.height
  raw_size = mask_offset + mask_size
  color_stride = encoded.blocks_x * 2
  color_height = encoded.blocks_y

  return f"""#pragma once

/*
  Generated by ft812_dxt_convert_v2.py.

  Format: {spec.name}

  RAM_G raw layout:
    c0 RGB565 layer + c1 RGB565 layer + {spec.mask_format} mask

  FT812 display-list helper notes:
    - color bitmap handle points to c0+c1 RGB565 cells
    - cell 0 = c0
    - cell 1 = c1
    - mask bitmap handle points to L2/L4 mask
    - first draw mask so it writes alpha
    - then draw c1 with DST_ALPHA
    - then draw c0 with ONE_MINUS_DST_ALPHA
*/

#define {prefix}_WIDTH {encoded.width}
#define {prefix}_HEIGHT {encoded.height}
#define {prefix}_BLOCKS_X {encoded.blocks_x}
#define {prefix}_BLOCKS_Y {encoded.blocks_y}
#define {prefix}_C0_OFFSET {c0_offset}
#define {prefix}_C1_OFFSET {c1_offset}
#define {prefix}_MASK_OFFSET {mask_offset}
#define {prefix}_C0_SIZE {c0_size}
#define {prefix}_C1_SIZE {c1_size}
#define {prefix}_MASK_SIZE {mask_size}
#define {prefix}_RAW_SIZE {raw_size}
#define {prefix}_COLOR_STRIDE {color_stride}
#define {prefix}_COLOR_HEIGHT {color_height}
#define {prefix}_MASK_STRIDE {mask_stride}
#define {prefix}_FORMAT_TYPE {spec.type_id}
#define {prefix}_MASK_FORMAT {spec.mask_format}
"""


def write_file(path: Path, data: bytes | str) -> None:
  path.parent.mkdir(parents=True, exist_ok=True)
  if isinstance(data, str):
    path.write_text(data, encoding="utf-8")
  else:
    path.write_bytes(data)


def write_outputs(encoded: EncodedImage, output_base: str, out_kinds: set[str], split: bool,
                  preview: bool, zlib_level: int, progress: Progress | None = None,
                  gpu_evaluator: OpenCLBlockEvaluator | None = None) -> list[Path]:
  suffix = encoded.spec.suffix
  output_base_path = Path(output_base)
  output_prefix = output_base_path.parent / f"{output_base_path.name}_{suffix}"
  files: list[Path] = []

  total_steps = 0
  if "raw" in out_kinds:
    total_steps += 2
    if split:
      total_steps += 3
  if "dxt" in out_kinds:
    total_steps += 1
  if "dxp" in out_kinds:
    total_steps += 1
  if preview:
    total_steps += 1

  done_steps = 0
  progress_label = f"{suffix} write"
  if progress is not None:
    progress.show(progress_label, 0, total_steps, force=True)

  raw: bytes | None = None

  def get_raw() -> bytes:
    nonlocal raw
    if raw is None:
      if gpu_evaluator is not None:
        raw = gpu_evaluator.pack_raw_from_encoded(encoded)
      else:
        raw = make_raw(encoded)
    return raw

  if "raw" in out_kinds:
    header_path = Path(f"{output_prefix}.h")
    write_file(header_path, header_text(encoded, output_base))
    files.append(header_path)
    done_steps += 1
    if progress is not None:
      progress.show(progress_label, done_steps, total_steps)

    raw_path = Path(f"{output_prefix}.raw")
    raw_data = get_raw()
    write_file(raw_path, raw_data)
    files.append(raw_path)
    done_steps += 1
    if progress is not None:
      progress.show(progress_label, done_steps, total_steps)

    if split:
      raw_data = get_raw()
      c0_size = len(encoded.blocks) * 2
      c1_size = c0_size
      c0 = raw_data[:c0_size]
      c1 = raw_data[c0_size:c0_size + c1_size]
      mask = raw_data[c0_size + c1_size:]
      c0_path = Path(f"{output_prefix}_c0.raw")
      c1_path = Path(f"{output_prefix}_c1.raw")
      mask_path = Path(f"{output_prefix}_{suffix}.raw")
      write_file(c0_path, c0)
      files.append(c0_path)
      done_steps += 1
      if progress is not None:
        progress.show(progress_label, done_steps, total_steps)
      write_file(c1_path, c1)
      files.append(c1_path)
      done_steps += 1
      if progress is not None:
        progress.show(progress_label, done_steps, total_steps)
      write_file(mask_path, mask)
      files.append(mask_path)
      done_steps += 1
      if progress is not None:
        progress.show(progress_label, done_steps, total_steps)

  if "dxt" in out_kinds:
    dxt_path = Path(f"{output_prefix}.dxt")
    write_file(dxt_path, make_dxt(encoded))
    files.append(dxt_path)
    done_steps += 1
    if progress is not None:
      progress.show(progress_label, done_steps, total_steps)

  if "dxp" in out_kinds:
    if encoded.width > 65535 or encoded.height > 65535:
      raise CliError("DXP header stores width/height as uint16, image is too large")
    dxp_path = Path(f"{output_prefix}.dxp")
    write_file(dxp_path, make_dxp(encoded, get_raw(), zlib_level))
    files.append(dxp_path)
    done_steps += 1
    if progress is not None:
      progress.show(progress_label, done_steps, total_steps)

  if preview:
    preview_path = Path(f"{output_prefix}_preview.png")
    preview_path.parent.mkdir(parents=True, exist_ok=True)
    if gpu_evaluator is not None:
      gpu_evaluator.make_preview_from_encoded(encoded).save(preview_path)
    else:
      make_preview(encoded).save(preview_path)
    files.append(preview_path)
    done_steps += 1
    if progress is not None:
      progress.show(progress_label, done_steps, total_steps)

  return files

def parse_background(text: str) -> tuple[int, int, int]:
  if not re.fullmatch(r"#[0-9a-fA-F]{6}", text):
    raise CliError("--background must be in #RRGGBB format")
  return int(text[1:3], 16), int(text[3:5], 16), int(text[5:7], 16)


def load_image(path: Path, background: tuple[int, int, int], size_mode: str) -> Image.Image:
  if not path.exists():
    raise CliError(f"input file does not exist: {path}")

  try:
    with Image.open(path) as src:
      rgba = src.convert("RGBA")
  except Exception as exc:
    raise CliError(f"failed to open image: {path}: {exc}") from exc

  bg = Image.new("RGBA", rgba.size, background + (255,))
  image = Image.alpha_composite(bg, rgba).convert("RGB")
  width, height = image.size

  if width <= 0 or height <= 0:
    raise CliError("image has invalid zero size")

  if width % 4 == 0 and height % 4 == 0:
    return image

  if size_mode == "error":
    raise CliError(f"image size must be divisible by 4, got {width}x{height}; use --size-mode crop or pad")

  if size_mode == "crop":
    new_w = width - (width % 4)
    new_h = height - (height % 4)
    if new_w <= 0 or new_h <= 0:
      raise CliError(f"cropped image would be empty, got source size {width}x{height}")
    return image.crop((0, 0, new_w, new_h))

  if size_mode == "pad":
    new_w = ((width + 3) // 4) * 4
    new_h = ((height + 3) // 4) * 4
    padded = Image.new("RGB", (new_w, new_h), background)
    padded.paste(image, (0, 0))
    return padded

  raise CliError(f"unknown size mode: {size_mode}")


def parse_csv_option(text: str, allowed: set[str], default_all: set[str], opt_name: str) -> set[str]:
  value = text.strip().lower()
  if value == "all":
    return set(default_all)
  parts = [part.strip().lower() for part in value.split(",") if part.strip()]
  if not parts:
    raise CliError(f"{opt_name} must not be empty")
  bad = [part for part in parts if part not in allowed]
  if bad:
    raise CliError(f"invalid {opt_name} value: {', '.join(bad)}")
  return set(parts)


def build_arg_parser() -> argparse.ArgumentParser:
  parser = argparse.ArgumentParser(
    description="Convert images to FT812 DXT1_L2_RGB565 / DXT1_L4_RGB565 raw, dxt, and dxp files.",
    formatter_class=argparse.RawDescriptionHelpFormatter,
    epilog=EFFORT_LEVEL_HELP,
  )
  parser.add_argument("input", help="input image file supported by Pillow")
  parser.add_argument("-o", "--output-dir", dest="output_dir", default=None, help="output directory; file prefix is taken from input file name")
  parser.add_argument("--output-base", dest="output_dir", help=argparse.SUPPRESS)
  parser.add_argument("-f", "--format", default="l2,l4", help="l2 / l4 / l2,l4 / all, default: l2,l4")
  parser.add_argument("-t", "--out", default="raw,dxt,dxp", help="raw / dxt / dxp / raw,dxt,dxp / all, default: raw,dxt,dxp")
  parser.add_argument("-e", "--effort", type=int, default=4, help="encoder optimization level 0..10, see levels below, default: 4")
  parser.add_argument("-j", "--jobs", type=int, default=0, help="worker processes, 0 = os.cpu_count(), 1 = no multiprocessing, default: 0")
  parser.add_argument("-g", "--gpu", dest="gpu", action="store_true", default=True, help="use optional OpenCL GPU selector evaluation and hybrid GPU refine through numpy + pyopencl; enabled by default")
  parser.add_argument("-c", "--cpu", dest="gpu", action="store_false", help="disable GPU and use CPU encoding")
  parser.add_argument("--gpu-batch", type=int, default=262144, help="OpenCL evaluation batch size for -g, default: 262144")
  parser.add_argument("--profile", action="store_true", help="print debug timing/counter metrics to stderr")
  parser.add_argument("--select-candidates", type=int, default=0, help="max candidates per block tested by seam select, 0 = auto, default: 0")
  parser.add_argument("--select-residual-candidates", type=int, default=0, help="max candidates per block tested by residual select, 0 = auto, default: 0")
  parser.add_argument("--no-select-early-stop", dest="select_early_stop", action="store_false", help="disable early stop in seam-aware select passes")
  parser.add_argument("--select-residual-all", dest="select_residual_active", action="store_false", help="test residual select on all blocks instead of only active seam-neighborhood blocks")
  parser.set_defaults(select_early_stop=True, select_residual_active=True)
  parser.add_argument("--select-early-stop-threshold", type=int, default=0, help="absolute changed-block threshold for select early stop, 0 = auto, default: 0")
  parser.add_argument("--select-early-stop-ratio", type=int, default=96, help="auto threshold divisor: max(32, blocks/ratio), default: 96")
  parser.add_argument("-z", "--zlib-level", type=int, default=9, help="zlib level for DXP 0..9, default: 9")
  parser.add_argument("-m", "--size-mode", choices=("error", "crop", "pad"), default="error", help="size handling for non-4x4 images, default: error")
  parser.add_argument("-b", "--background", default="#000000", help="background #RRGGBB for alpha composite and padding, default: #000000")
  parser.add_argument("-x", "--split", action="store_true", help="write separate raw layer files")
  parser.add_argument("-p", "--preview", action="store_true", help="write reconstructed preview PNG")
  return parser


def make_output_base(input_path: Path, output_dir_arg: str | None) -> str:
  stem = input_path.stem
  if not stem:
    raise CliError("input file name must have a non-empty stem")

  if output_dir_arg is None:
    output_dir = input_path.parent
  else:
    output_dir = Path(output_dir_arg)

  return str(output_dir / stem)


def validate_args(args: argparse.Namespace) -> tuple[Path, str, set[str], set[str], tuple[int, int, int]]:
  input_path = Path(args.input)
  output_base = make_output_base(input_path, args.output_dir)

  if args.effort < 0 or args.effort > 10:
    raise CliError("--effort must be in range 0..10")
  if args.jobs < 0:
    raise CliError("--jobs must be >= 0")
  if args.gpu_batch <= 0:
    raise CliError("--gpu-batch must be > 0")
  if args.select_candidates < 0:
    raise CliError("--select-candidates must be >= 0")
  if args.select_residual_candidates < 0:
    raise CliError("--select-residual-candidates must be >= 0")
  if args.select_early_stop_threshold < 0:
    raise CliError("--select-early-stop-threshold must be >= 0")
  if args.select_early_stop_ratio <= 0:
    raise CliError("--select-early-stop-ratio must be > 0")
  if args.zlib_level < 0 or args.zlib_level > 9:
    raise CliError("--zlib-level must be in range 0..9")

  formats = parse_csv_option(args.format, {"l2", "l4"}, {"l2", "l4"}, "--format")
  out_kinds = parse_csv_option(args.out, {"raw", "dxt", "dxp"}, {"raw", "dxt", "dxp"}, "--out")
  background = parse_background(args.background)
  return input_path, output_base, formats, out_kinds, background


def print_run_parameters(args: argparse.Namespace, input_path: Path, output_base: str, formats: set[str],
                         out_kinds: set[str], background: tuple[int, int, int]) -> None:
  gpu_note = "on" if args.gpu else "off"
  print("parameters:", flush=True)
  print(f"  input: {input_path}", flush=True)
  print(f"  output_base: {output_base}", flush=True)
  print(f"  format: {','.join(sorted(formats))}", flush=True)
  print(f"  out: {','.join(sorted(out_kinds))}", flush=True)
  print(f"  effort: {args.effort}", flush=True)
  print(f"  jobs: {args.jobs}", flush=True)
  print(f"  gpu: {gpu_note}", flush=True)
  print(f"  gpu_batch: {args.gpu_batch}", flush=True)
  print(f"  zlib_level: {args.zlib_level}", flush=True)
  print(f"  size_mode: {args.size_mode}", flush=True)
  print(f"  background: #{background[0]:02x}{background[1]:02x}{background[2]:02x}", flush=True)
  print(f"  split: {args.split}", flush=True)
  print(f"  preview: {args.preview}", flush=True)
  print(f"  profile: {args.profile}", flush=True)
  print(f"  select_candidates: {args.select_candidates}", flush=True)
  print(f"  select_residual_candidates: {args.select_residual_candidates}", flush=True)
  print(f"  select_early_stop: {args.select_early_stop}", flush=True)
  print(f"  select_early_stop_threshold: {args.select_early_stop_threshold}", flush=True)
  print(f"  select_early_stop_ratio: {args.select_early_stop_ratio}", flush=True)
  print(f"  select_residual_active: {args.select_residual_active}", flush=True)


def run(args: argparse.Namespace) -> int:
  profile = ProfileMetrics(enabled=args.profile)
  total_t0 = time.perf_counter()
  input_path, output_base, formats, out_kinds, background = validate_args(args)
  print_run_parameters(args, input_path, output_base, formats, out_kinds, background)
  load_t0 = time.perf_counter()
  image = load_image(input_path, background, args.size_mode)
  profile.add_time("load_image", time.perf_counter() - load_t0)
  width, height = image.size

  if width <= 0 or height <= 0 or width % 4 != 0 or height % 4 != 0:
    raise CliError(f"internal size validation failed after {args.size_mode}: {width}x{height}")

  rgb_bytes = image.tobytes()
  blocks_x = width // 4
  blocks_y = height // 4
  extract_t0 = time.perf_counter()
  blocks = extract_blocks(rgb_bytes, width, height)
  profile.add_time("extract_blocks", time.perf_counter() - extract_t0)
  profile.add_count("image.width", width)
  profile.add_count("image.height", height)
  profile.add_count("image.blocks", len(blocks))

  output_files: list[Path] = []
  progress = Progress()
  select_options = SelectOptions(
    seam_candidates=args.select_candidates,
    residual_candidates=args.select_residual_candidates,
    early_stop=args.select_early_stop,
    early_stop_threshold=args.select_early_stop_threshold,
    early_stop_ratio=args.select_early_stop_ratio,
    residual_active=args.select_residual_active,
  )
  gpu_initial_pairs: list[list[tuple[int, int]]] | None = None
  gpu_evaluator: OpenCLBlockEvaluator | None = None
  if args.gpu:
    gpu_evaluator = OpenCLBlockEvaluator(blocks, profile)
    print(f"OpenCL device: {gpu_evaluator.device_name}", file=sys.stderr, flush=True)
    gpu_weights = PERCEPTUAL_WEIGHTS if effort_config(args.effort).perceptual else RGB_WEIGHTS
    gpu_initial_pairs = build_initial_pair_cache_gpu(blocks, args.effort, gpu_weights, gpu_evaluator, profile)

  for spec_key in ("l2", "l4"):
    if spec_key not in formats:
      continue
    encode_t0 = time.perf_counter()
    encoded = encode_image_blocks(
      blocks,
      blocks_x,
      blocks_y,
      spec_key,
      args.effort,
      args.jobs,
      progress,
      args.gpu,
      args.gpu_batch,
      profile,
      gpu_initial_pairs,
      gpu_evaluator,
      select_options,
    )
    profile.add_time(f"{spec_key}.encode_call", time.perf_counter() - encode_t0)
    write_t0 = time.perf_counter()
    output_files.extend(write_outputs(
      encoded,
      output_base,
      out_kinds,
      args.split,
      args.preview,
      args.zlib_level,
      progress,
      gpu_evaluator if args.gpu else None,
    ))
    profile.add_time(f"{spec_key}.write_outputs", time.perf_counter() - write_t0)

  print(f"input: {input_path}")
  print(f"size: {width}x{height}, blocks: {blocks_x}x{blocks_y}")
  gpu_note = "on" if args.gpu else "off"
  print(f"effort: {args.effort}, jobs: {args.jobs}, gpu: {gpu_note}, gpu_batch: {args.gpu_batch}, zlib: {args.zlib_level}")
  for path in output_files:
    print(f"wrote: {path}")
  profile.add_time("total", time.perf_counter() - total_t0)
  profile.show()
  return 0


def main(argv: Sequence[str] | None = None) -> int:
  parser = build_arg_parser()
  args = parser.parse_args(argv)
  try:
    return run(args)
  except CliError as exc:
    parser.exit(2, f"error: {exc}\n")


if __name__ == "__main__":
  raise SystemExit(main())
