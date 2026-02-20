#!/usr/bin/env python3
from __future__ import annotations

import argparse
import math
import os
import struct
import sys
from dataclasses import dataclass
from typing import List, Optional, Tuple

SECTOR_SIZE = 256
SECTORS_PER_TRACK = 16
TRACK_SIZE = SECTOR_SIZE * SECTORS_PER_TRACK  # 4096
DIRENT_SIZE = 16
MAX_DIRENTS = 128
DIR_BYTES = MAX_DIRENTS * DIRENT_SIZE  # 2048

VINFO_OFF = 0x800  # disk info sector begins right after directory area

# Offsets inside disk info sector (relative to VINFO_OFF)
OFF_FIRST_FREE_SECTOR = 0xE1  # 1 byte
OFF_FIRST_FREE_TRACK = 0xE2   # 1 byte
OFF_DISK_TYPE = 0xE3          # 1 byte
OFF_N_FILES = 0xE4            # 1 byte (commonly "including deleted") :contentReference[oaicite:2]{index=2}
OFF_N_FREE_SECTORS = 0xE5     # 2 bytes LE
OFF_TRDOS_ID = 0xE7           # 1 byte, should be 0x10 :contentReference[oaicite:3]{index=3}
OFF_N_DELETED = 0xF4          # 1 byte
OFF_LABEL = 0xF5              # 8 bytes

TRDOS_ID = 0x10

DISK_TYPE_BY_GEOM = {
    (80, 2): 0x16,
    (40, 2): 0x17,
    (80, 1): 0x18,
    (40, 1): 0x19,
}
GEOM_BY_DISK_TYPE = {v: k for k, v in DISK_TYPE_BY_GEOM.items()}


def _u16le(b: bytes) -> int:
    return struct.unpack("<H", b)[0]


def _p16le(v: int) -> bytes:
    return struct.pack("<H", v & 0xFFFF)


def _is_printable_ascii(x: int) -> bool:
    return 0x20 <= x <= 0x7E


def _fmt_ext(ext: int) -> str:
    if _is_printable_ascii(ext):
        return chr(ext)
    return f"\\x{ext:02X}"


def _decode_name(name8: bytes) -> str:
    # TR-DOS uses ZX charset, но ASCII диапазон в целом совместим. :contentReference[oaicite:4]{index=4}
    return name8.rstrip(b" ").decode("ascii", errors="replace")


def _encode_name8(name: str) -> bytes:
    s = name.strip().upper()
    raw = s.encode("ascii", errors="replace")[:8]
    return raw.ljust(8, b" ")


def parse_trdos_filename(s: str) -> Tuple[bytes, int]:
    """
    Accepts:
      - "NAME.C" (one-char ext)
      - "NAME"   (defaults to ext 'C')
    """
    s = s.strip()
    if "." in s:
        base, ext = s.rsplit(".", 1)
        ext = ext.strip()
        if len(ext) != 1:
            raise ValueError("TR-DOS extension must be exactly 1 character (e.g. NAME.C)")
        ext_b = ext.upper().encode("ascii", errors="replace")[0]
    else:
        base = s
        ext_b = ord("C")
    return _encode_name8(base), ext_b


def lba_offset(track: int, sector: int) -> int:
    # track and sector are logical; sector is 0..15. :contentReference[oaicite:5]{index=5}
    return (track * SECTORS_PER_TRACK + sector) * SECTOR_SIZE


def advance_ts(track: int, sector: int, n_sectors: int) -> Tuple[int, int]:
    idx = track * SECTORS_PER_TRACK + sector
    idx += n_sectors
    return idx // SECTORS_PER_TRACK, idx % SECTORS_PER_TRACK


@dataclass
class DirEnt:
    idx: int
    name8: bytes
    ext: int
    p1: int  # bytes 9-10
    p2: int  # bytes 11-12
    len_sectors: int
    start_sector: int
    start_track: int

    @property
    def is_terminator(self) -> bool:
        return self.name8[:1] == b"\x00"

    @property
    def is_deleted(self) -> bool:
        return self.name8[:1] == b"\x01"

    @property
    def display_name(self) -> str:
        return f"{_decode_name(self.name8)}.{_fmt_ext(self.ext)}"

    def pack(self) -> bytes:
        b = bytearray(DIRENT_SIZE)
        b[0:8] = self.name8
        b[8] = self.ext & 0xFF
        b[9:11] = _p16le(self.p1)
        b[11:13] = _p16le(self.p2)
        b[13] = self.len_sectors & 0xFF
        b[14] = self.start_sector & 0xFF
        b[15] = self.start_track & 0xFF
        return bytes(b)


@dataclass
class DiskInfo:
    first_free_sector: int
    first_free_track: int
    disk_type: int
    n_files: int
    n_free_sectors: int
    n_deleted: int
    label8: bytes

    @property
    def geom(self) -> Optional[Tuple[int, int]]:
        return GEOM_BY_DISK_TYPE.get(self.disk_type)

    @property
    def total_logical_tracks(self) -> Optional[int]:
        g = self.geom
        if not g:
            return None
        tracks_per_side, sides = g
        return tracks_per_side * sides

    @property
    def total_sectors(self) -> Optional[int]:
        t = self.total_logical_tracks
        if t is None:
            return None
        return t * SECTORS_PER_TRACK


class TRDImage:
    def __init__(self, data: bytearray):
        self.data = data
        self.info = self._read_disk_info()

    @staticmethod
    def load(path: str) -> "TRDImage":
        with open(path, "rb") as f:
            data = bytearray(f.read())
        img = TRDImage(data)
        return img

    def save(self, path: str) -> None:
        with open(path, "wb") as f:
            f.write(self.data)

    def ensure_len(self, size: int) -> None:
        if len(self.data) < size:
            self.data.extend(b"\x00" * (size - len(self.data)))

    def _read_disk_info(self) -> DiskInfo:
        self.ensure_len(VINFO_OFF + SECTOR_SIZE)
        s = self.data[VINFO_OFF:VINFO_OFF + SECTOR_SIZE]
        first_free_sector = s[OFF_FIRST_FREE_SECTOR]
        first_free_track = s[OFF_FIRST_FREE_TRACK]
        disk_type = s[OFF_DISK_TYPE]
        n_files = s[OFF_N_FILES]
        n_free_sectors = _u16le(s[OFF_N_FREE_SECTORS:OFF_N_FREE_SECTORS + 2])
        trdos_id = s[OFF_TRDOS_ID]
        n_deleted = s[OFF_N_DELETED]
        label8 = bytes(s[OFF_LABEL:OFF_LABEL + 8])
        if trdos_id not in (0x00, TRDOS_ID):
            # Unusual, but don't hard-fail: some images are "dirty".
            pass
        return DiskInfo(
            first_free_sector=first_free_sector,
            first_free_track=first_free_track,
            disk_type=disk_type,
            n_files=n_files,
            n_free_sectors=n_free_sectors,
            n_deleted=n_deleted,
            label8=label8,
        )

    def _write_disk_info(self, info: DiskInfo) -> None:
        self.ensure_len(VINFO_OFF + SECTOR_SIZE)
        base = VINFO_OFF
        self.data[base + OFF_FIRST_FREE_SECTOR] = info.first_free_sector & 0xFF
        self.data[base + OFF_FIRST_FREE_TRACK] = info.first_free_track & 0xFF
        self.data[base + OFF_DISK_TYPE] = info.disk_type & 0xFF
        self.data[base + OFF_N_FILES] = info.n_files & 0xFF
        self.data[base + OFF_N_FREE_SECTORS:base + OFF_N_FREE_SECTORS + 2] = _p16le(info.n_free_sectors)
        self.data[base + OFF_TRDOS_ID] = TRDOS_ID
        self.data[base + OFF_N_DELETED] = info.n_deleted & 0xFF
        self.data[base + OFF_LABEL:base + OFF_LABEL + 8] = info.label8[:8].ljust(8, b" ")

        # "BLANK9" / password area typically spaces. :contentReference[oaicite:6]{index=6}
        # Keep existing bytes if already non-zero; only normalize on create().

        self.info = info

    def iter_dir(self, limit: Optional[int] = None) -> List[DirEnt]:
        # Prefer using n_files as "used entries including deleted" (common in docs). :contentReference[oaicite:7]{index=7}
        n = self.info.n_files if limit is None else min(limit, MAX_DIRENTS)
        n = min(n, MAX_DIRENTS)
        out: List[DirEnt] = []
        for i in range(n):
            off = i * DIRENT_SIZE
            raw = self.data[off:off + DIRENT_SIZE]
            name8 = bytes(raw[0:8])
            ext = raw[8]
            p1 = _u16le(raw[9:11])
            p2 = _u16le(raw[11:13])
            len_sectors = raw[13]
            start_sector = raw[14]
            start_track = raw[15]
            out.append(DirEnt(i, name8, ext, p1, p2, len_sectors, start_sector, start_track))
        return out

    def find_entry(self, trdos_name: str) -> DirEnt:
        name8, ext = parse_trdos_filename(trdos_name)
        for e in self.iter_dir():
            if e.is_terminator:
                break
            if e.is_deleted:
                continue
            if e.name8 == name8 and e.ext == ext:
                return e
        raise KeyError(f"Not found: {trdos_name}")

    def list(self) -> None:
        entries = self.iter_dir()
        active = 0
        deleted = 0
        for e in entries:
            if e.is_terminator:
                break
            if e.is_deleted:
                deleted += 1
                continue
            active += 1
            print(f"{e.display_name:12}  sectors = {e.len_sectors}  start: trk = {e.start_track} sec = {e.start_sector}")

        print(f"\nActive: {active}, Deleted: {deleted} (info.n_deleted={self.info.n_deleted})")
        print(f"Free: {self.info.n_free_sectors} sectors, Next free: t{self.info.first_free_track}:s{self.info.first_free_sector}")

    def _max_capacity_sectors(self) -> int:
        # If disk_type known, trust it; else infer from current file length.
        total = self.info.total_sectors
        if total is not None:
            return total
        return (len(self.data) // SECTOR_SIZE)

    def _reserved_sectors(self) -> int:
        # Track 0 is reserved for dir+info; 1 logical track = 16 sectors. :contentReference[oaicite:8]{index=8}
        return SECTORS_PER_TRACK

    def add_file(self, host_path: str, trdos_name: str, load_addr: int = 0, force: bool = False) -> None:
        if not os.path.isfile(host_path):
            raise FileNotFoundError(host_path)
        with open(host_path, "rb") as f:
            payload = f.read()

        name8, ext = parse_trdos_filename(trdos_name)

        # Simple duplicate guard
        try:
            _ = self.find_entry(trdos_name)
            if not force:
                raise FileExistsError(f"File already exists in image: {trdos_name} (use --force to add anyway)")
        except KeyError:
            pass

        need_sectors = math.ceil(len(payload) / SECTOR_SIZE)
        if need_sectors <= 0:
            need_sectors = 1
            payload = b""

        if need_sectors > 255:
            raise ValueError("Too large for 1-byte sector count (len_sectors). Split or use other container format.")

        if need_sectors > self.info.n_free_sectors:
            raise ValueError(f"Not enough free space: need {need_sectors} sectors, have {self.info.n_free_sectors}")

        # Directory slot = current n_files (including deleted)
        if self.info.n_files >= MAX_DIRENTS:
            raise ValueError("Directory full (128 entries).")
        dir_idx = self.info.n_files

        # Start position from disk info
        t = self.info.first_free_track
        s = self.info.first_free_sector

        # Ensure we don't write past expected geometry (if known)
        start_abs = t * SECTORS_PER_TRACK + s
        end_abs = start_abs + need_sectors
        if self.info.total_sectors is not None and end_abs > self.info.total_sectors:
            raise ValueError("Write would exceed disk geometry from disk_type.")

        # Ensure file is large enough
        self.ensure_len(end_abs * SECTOR_SIZE)

        # Write data, pad last sector with zeros
        write_off = lba_offset(t, s)
        padded = payload.ljust(need_sectors * SECTOR_SIZE, b"\x00")
        self.data[write_off:write_off + len(padded)] = padded

        # Build dirent fields:
        # For CODE ('C'): bytes 9-10 = load address, 11-12 = length. :contentReference[oaicite:9]{index=9}
        p1 = load_addr & 0xFFFF
        p2 = len(payload) & 0xFFFF

        ent = DirEnt(
            idx=dir_idx,
            name8=name8,
            ext=ext,
            p1=p1,
            p2=p2,
            len_sectors=need_sectors,
            start_sector=s,
            start_track=t,
        )

        # Write entry
        off = dir_idx * DIRENT_SIZE
        self.data[off:off + DIRENT_SIZE] = ent.pack()

        # Update disk info
        nt, ns = advance_ts(t, s, need_sectors)
        info = self.info
        info.n_files = (info.n_files + 1) & 0xFF
        info.n_free_sectors = max(0, info.n_free_sectors - need_sectors)
        info.first_free_track = nt & 0xFF
        info.first_free_sector = ns & 0xFF
        self._write_disk_info(info)

    def delete_file(self, trdos_name: str) -> None:
        e = self.find_entry(trdos_name)

        # Mark deleted: first byte of filename = 0x01 :contentReference[oaicite:10]{index=10}
        off = e.idx * DIRENT_SIZE
        self.data[off + 0] = 0x01

        info = self.info
        info.n_deleted = (info.n_deleted + 1) & 0xFF
        # Note: sectors are not reclaimed until MOVE/compact. :contentReference[oaicite:11]{index=11}
        self._write_disk_info(info)

    def compact(self) -> None:
        old = bytes(self.data)  # immutable snapshot
        entries = self.iter_dir()

        active: List[DirEnt] = []
        for e in entries:
            if e.is_terminator:
                break
            if e.is_deleted:
                continue
            active.append(e)

        # Prepare new image: keep same size, but clear data area (from logical track 1)
        new_data = bytearray(old)
        data_start = self._reserved_sectors() * SECTOR_SIZE
        for i in range(data_start, len(new_data)):
            new_data[i] = 0

        # Pack files starting at t=1,s=0 (default next-free after formatting) :contentReference[oaicite:12]{index=12}
        t, s = 1, 0
        used_sectors = 0

        new_dir: List[DirEnt] = []
        for new_idx, e in enumerate(active):
            src_off = lba_offset(e.start_track, e.start_sector)
            size = e.len_sectors * SECTOR_SIZE
            dst_off = lba_offset(t, s)

            end_abs = (t * SECTORS_PER_TRACK + s) + e.len_sectors
            if self.info.total_sectors is not None and end_abs > self.info.total_sectors:
                raise ValueError("Compaction would exceed disk geometry from disk_type.")

            if dst_off + size > len(new_data):
                # extend if needed
                need = dst_off + size
                new_data.extend(b"\x00" * (need - len(new_data)))

            new_data[dst_off:dst_off + size] = old[src_off:src_off + size]

            ne = DirEnt(
                idx=new_idx,
                name8=e.name8,
                ext=e.ext,
                p1=e.p1,
                p2=e.p2,
                len_sectors=e.len_sectors,
                start_sector=s,
                start_track=t,
            )
            new_dir.append(ne)

            used_sectors += e.len_sectors
            t, s = advance_ts(t, s, e.len_sectors)

        # Rebuild directory area (first 8 sectors)
        # 1) Clear directory area
        for i in range(0, DIR_BYTES):
            new_data[i] = 0

        # 2) Write active entries
        for e in new_dir:
            off = e.idx * DIRENT_SIZE
            new_data[off:off + DIRENT_SIZE] = e.pack()

        # Terminator: next entry name[0] = 0x00 already (cleared), OK.

        # Update disk info
        info = self.info
        info.n_files = len(new_dir) & 0xFF
        info.n_deleted = 0

        # first free pointer
        info.first_free_track = t & 0xFF
        info.first_free_sector = s & 0xFF

        # free sectors = total - reserved - used
        total = self._max_capacity_sectors()
        info.n_free_sectors = max(0, total - self._reserved_sectors() - used_sectors)

        # Normalize TR-DOS ID
        base = VINFO_OFF
        new_data[base + OFF_TRDOS_ID] = TRDOS_ID

        self.data = new_data
        self._write_disk_info(info)


def cmd_create(args: argparse.Namespace) -> None:
    tracks = args.tracks
    sides = args.sides
    if (tracks, sides) not in DISK_TYPE_BY_GEOM:
        raise SystemExit("create supports only: tracks=40/80 and sides=1/2 (standard TR-DOS disk types).")
    disk_type = DISK_TYPE_BY_GEOM[(tracks, sides)]

    total_logical_tracks = tracks * sides
    size = total_logical_tracks * TRACK_SIZE
    data = bytearray(b"\x00" * size)

    # Directory is empty => first entry first byte 0x00 (already)

    # Disk info sector initialization
    # First byte 0 == end-of-catalog marker position. :contentReference[oaicite:13]{index=13}
    base = VINFO_OFF
    data[base + 0] = 0x00
    # 224 bytes unused: keep zeros :contentReference[oaicite:14]{index=14}

    # next free sector/track (init sec=0, track=1) :contentReference[oaicite:15]{index=15}
    data[base + OFF_FIRST_FREE_SECTOR] = 0x00
    data[base + OFF_FIRST_FREE_TRACK] = 0x01

    data[base + OFF_DISK_TYPE] = disk_type
    data[base + OFF_N_FILES] = 0x00

    total_sectors = total_logical_tracks * SECTORS_PER_TRACK
    free_sectors = total_sectors - SECTORS_PER_TRACK  # minus track0 reserved
    data[base + OFF_N_FREE_SECTORS:base + OFF_N_FREE_SECTORS + 2] = _p16le(free_sectors)

    data[base + OFF_TRDOS_ID] = TRDOS_ID

    # password/blank9 area (9 spaces) commonly used :contentReference[oaicite:16]{index=16}
    # Sinclair wiki marks it as spaces/unusued. We'll set spaces.
    data[base + 0xE9:base + 0xE9 + 9] = b" " * 9

    data[base + OFF_N_DELETED] = 0x00

    label = (args.label or "").encode("ascii", errors="replace")[:8].ljust(8, b" ")
    data[base + OFF_LABEL:base + OFF_LABEL + 8] = label

    TRDImage(data).save(args.image)


def cmd_list(args: argparse.Namespace) -> None:
    img = TRDImage.load(args.image)
    img.list()


def cmd_add(args: argparse.Namespace) -> None:
    img = TRDImage.load(args.image)
    img.add_file(args.host_file, args.trd_name, load_addr=args.load, force=args.force)
    img.save(args.image)


def cmd_del(args: argparse.Namespace) -> None:
    img = TRDImage.load(args.image)
    img.delete_file(args.trd_name)
    img.save(args.image)


def cmd_compact(args: argparse.Namespace) -> None:
    img = TRDImage.load(args.image)
    img.compact()
    img.save(args.image)


class Formatter(argparse.RawDescriptionHelpFormatter, argparse.ArgumentDefaultsHelpFormatter):
    pass


def build_argparser() -> argparse.ArgumentParser:
    top_examples = """Examples:
  # Create a standard 80-track, double-side TRD with label
  trdtool create disk.trd --tracks 80 --sides 2 --label DEMO

  # List files in image
  trdtool list disk.trd

  # Add a host file as TR-DOS CODE (extension .C by default if omitted)
  trdtool add disk.trd demo.bin DEMO.C --load 0x8000
  trdtool add disk.trd demo.bin DEMO --load 0x8000

  # Delete a file (marks directory entry as deleted)
  trdtool del disk.trd DEMO.C

  # Compact image (MOVE): packs files, removes deleted entries, updates free space
  trdtool compact disk.trd
"""

    p = argparse.ArgumentParser(
      prog="trdtool",
      formatter_class=Formatter,
      description="TR-DOS / BetaDisk .TRD image utility.",
      epilog=top_examples,
    )
    
    sub = p.add_subparsers(
      dest="cmd",
      required=True,
      metavar="COMMAND",
      title="commands",                # опционально
      description="Available commands" # опционально
    )

    # ---- create ----
    c = sub.add_parser(
        "create",
        formatter_class=Formatter,
        help="Create new empty TRD image (format)",
        description=(
            "Create a new empty .TRD image with standard TR-DOS geometry.\n"
            "\n"
            "Geometry options:\n"
            "  --tracks 40/80 and --sides 1/2\n"
            "These map to standard TR-DOS disk types.\n"
        ),
        epilog=(
            "Examples:\n"
            "  trdtool create disk.trd\n"
            "  trdtool create disk40.trd --tracks 40 --sides 2\n"
            "  trdtool create games.trd --label GAMES\n"
        ),
    )
    c.add_argument("image", metavar="IMAGE.trd", help="Path to .trd to create/overwrite")
    c.add_argument("--tracks", type=int, choices=[40, 80], default=80, help="Tracks per side")
    c.add_argument("--sides", type=int, choices=[1, 2], default=2, help="Number of sides")
    c.add_argument("--label", type=str, default="", help="Disk label (up to 8 ASCII chars)")
    c.set_defaults(func=cmd_create)

    # ---- list ----
    l = sub.add_parser(
        "list",
        formatter_class=Formatter,
        help="List directory entries",
        description=(
            "Show non-deleted directory entries in IMAGE.\n"
            "Also prints free space and next-free pointer from TR-DOS info sector.\n"
        ),
        epilog=(
            "Examples:\n"
            "  trdtool list disk.trd\n"
        ),
    )
    l.add_argument("image", metavar="IMAGE.trd", help="Path to .trd")
    l.set_defaults(func=cmd_list)

    # ---- add ----
    a = sub.add_parser(
        "add",
        formatter_class=Formatter,
        help="Add a host file into TRD image",
        description=(
            "Add HOST_FILE into IMAGE as a TR-DOS directory entry.\n"
            "\n"
            "TR-DOS filename rules (this tool):\n"
            "  - Name is up to 8 chars (will be uppercased and padded with spaces)\n"
            "  - Extension is exactly 1 char (e.g. .C, .B, .S ...)\n"
            "  - If extension is omitted, defaults to .C\n"
            "\n"
            "For CODE-type files (commonly ext 'C') this tool stores:\n"
            "  bytes 9-10  = load address (--load)\n"
            "  bytes 11-12 = original host file size (mod 65536)\n"
            "\n"
            "Notes:\n"
            "  - Space must be available as contiguous sectors; image is append-only\n"
            "    until you run 'compact'.\n"
            "  - If file with same name exists, use --force to still add.\n"
        ),
        epilog=(
            "Examples:\n"
            "  # Add binary as DEMO.C and set load address 0x8000\n"
            "  trdtool add disk.trd demo.bin DEMO.C --load 0x8000\n"
            "\n"
            "  # Same, but omit extension => defaults to .C\n"
            "  trdtool add disk.trd demo.bin DEMO --load 0x8000\n"
            "\n"
            "  # Add with a different 1-char extension\n"
            "  trdtool add disk.trd intro.scr INTRO.S\n"
            "\n"
            "  # Allow duplicate name\n"
            "  trdtool add disk.trd demo.bin DEMO.C --force\n"
        ),
    )
    a.add_argument("image", metavar="IMAGE.trd", help="Path to .trd")
    a.add_argument("host_file", metavar="HOST_FILE", help="Host file to add (raw bytes)")
    a.add_argument("trd_name", metavar="TRD_NAME", help='TR-DOS name like "NAME.C" (1-char ext) or "NAME" (defaults to .C)')
    a.add_argument("--load", type=lambda x: int(x, 0), default=0, help="Load address for CODE metadata (accepts 0x... or decimal)")
    a.add_argument("--force", action="store_true", help="Allow adding even if same TRD_NAME exists")
    a.set_defaults(func=cmd_add)

    # ---- del ----
    d = sub.add_parser(
        "del",
        formatter_class=Formatter,
        help="Delete a file entry (mark as deleted)",
        description=(
            "Mark TRD_NAME as deleted in directory.\n"
            "\n"
            "Important:\n"
            "  - This does NOT reclaim space immediately.\n"
            "  - Run 'compact' to pack files and reclaim free sectors.\n"
        ),
        epilog=(
            "Examples:\n"
            "  trdtool del disk.trd DEMO.C\n"
            "  trdtool del disk.trd INTRO.S\n"
        ),
    )
    d.add_argument("image", metavar="IMAGE.trd", help="Path to .trd")
    d.add_argument("trd_name", metavar="TRD_NAME", help='TR-DOS name like "NAME.C"')
    d.set_defaults(func=cmd_del)

    # ---- compact ----
    m = sub.add_parser(
        "compact",
        formatter_class=Formatter,
        help="Compact (MOVE): pack files and reclaim space",
        description=(
            "Repack all non-deleted files starting from track 1 sector 0.\n"
            "Deleted entries are removed; directory is rewritten.\n"
            "Updates free-space counters and next-free pointer.\n"
        ),
        epilog=(
            "Examples:\n"
            "  trdtool compact disk.trd\n"
            "\n"
            "Typical workflow:\n"
            "  trdtool del disk.trd OLD.C\n"
            "  trdtool compact disk.trd\n"
        ),
    )
    m.add_argument("image", metavar="IMAGE.trd", help="Path to .trd")
    m.set_defaults(func=cmd_compact)

    return p


def main(argv: List[str]) -> int:
    p = build_argparser()
    args = p.parse_args(argv)
    args.func(args)
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
