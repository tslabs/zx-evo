# ZiFi32: audio stream player

The ESP decodes a WAV, MP3, FLAC or Ogg Vorbis file that the host sends in pieces while the music plays, and mixes the samples into the 44.1 kHz output of `player_task`, with the tracker, the OPL and the SFX. The host does only file reads and SPI writes. Most of the time it just waits until the ESP has room for more.

The commands use the register protocol of `esp32-spi-wifi.md` (WR_REGS / RD_REGS / WR_DATA / W_END, status register 1). The codes are 0x90..0x97 (`esp_spi_defs.h`). A firmware without the player answers them with `ESP_ERR_INV_COMMAND` (0x80), so a host can probe for it with `AUDIO_GET_STATE`.

## Formats

| Code | Format | Decoder | Seek (FLUSH) |
|---|---|---|---|
| 0 | auto: from the first bytes of the stream | | |
| 1 | WAV (RIFF): PCM 8/16/24/32, float, A-law, u-law | dr_wav 0.14.6 | yes |
| 2 | MP3 (MPEG 1/2/2.5 Layer III, also I/II), an ID3v2 tag is skipped | dr_mp3 0.7.4 | yes |
| 3 | Ogg Vorbis | stb_vorbis 1.22 (pushdata) | no |
| 4 | FLAC (native) | dr_flac 0.13.4 | yes |

The decoders are single header files in `src/main/audio/third_party`, all public domain or MIT-0 (`README.md` there). Any sample rate is resampled to 44.1 kHz by linear interpolation. Mono is played on both channels; of more than two channels, the first two are played.

## Registers

Inputs:

| Reg | Size | Used by |
|---|---|---|
| 0x02 `ESP_REG_AUDIO_FORMAT` | 1 | AUDIO_START: format code (table above) |
| 0x02 `ESP_REG_AUDIO_PARAM` | 1 | AUDIO_PAUSE: 1 pause, 0 play on. AUDIO_VOLUME: attenuation in 0.5 dB steps, 0 = full, #FE = silence (as VS1053) |
| 0x03 `ESP_REG_DATA_SIZE` | 4 | AUDIO_PUSH: bytes in this piece, 1..16384 and not over FREE |
| 0x07 `ESP_REG_DATA_OFFSET` | 4 | AUDIO_FLUSH: file offset of the first byte of the next PUSH |

State, written by `AUDIO_GET_STATE`, `AUDIO_PUSH`, `AUDIO_END` and `AUDIO_PAUSE`:

| Reg | Size | Contents |
|---|---|---|
| 0x10 `STATE` | 1 | 0 IDLE (no stream), 1 WAIT (started, headers not decoded yet), 2 PLAY (also while paused), 3 DRAIN (decoded to the end, the last samples play), 4 ENDED, 5 ERROR |
| 0x11 `FMT` | 1 | format found (0 until the decoder has opened) |
| 0x12 `CHANNELS` | 1 | of the source |
| 0x13 `FLAGS` | 1 | bit 0 seekable, bit 1 paused, bit 2 END received |
| 0x14 `FREE` | 4 | bytes the next PUSHes may take |
| 0x18 `RATE` | 4 | sample rate of the source, Hz |
| 0x1C `PLAYED` | 4 | source frames played since START or FLUSH |
| 0x20 `TOTAL` | 4 | frames of the stream from its header (WAV, FLAC), 0 = not known (MP3, Ogg: the host finds the length itself, e.g. from a Xing / Info header) |
| 0x24 `BUFFERED` | 4 | host bytes not decoded yet |
| 0x28 `UNDERRUNS` | 2 | times the sound ran out while playing |
| 0x2A `ERROR` | 1 | in state ERROR: 0xC0 `ESP_ERR_AUDIO_FORMAT` (no format the player knows), 0xC1 `ESP_ERR_AUDIO_DECODE` (the decoder refused the headers) |
| 0x2B `BITS` | 1 | bits per sample of the source, 0 = compressed |

The elapsed time is PLAYED / RATE, and the length of the track is TOTAL / RATE.

## Commands

| Code | Command | |
|---|---|---|
| 0x90 | AUDIO_START | a new stream in FORMAT; stops the one before and empties the buffers |
| 0x91 | AUDIO_PUSH | DATA_SIZE bytes follow as a host DMA write (status M2S, then WR_DATA and W_END as for the other data commands). The status is READY when they are in, and the state registers are up to date |
| 0x92 | AUDIO_END | no more data: what is buffered plays out, then ENDED |
| 0x93 | AUDIO_FLUSH | seek: drops the buffered data and samples, and the next PUSH comes from the file offset DATA_OFFSET. Only when the SEEKABLE flag is set |
| 0x94 | AUDIO_PAUSE | PARAM 1 / 0 |
| 0x95 | AUDIO_VOLUME | PARAM attenuation |
| 0x96 | AUDIO_STOP | stops, IDLE |
| 0x97 | AUDIO_GET_STATE | the state registers |

PUSH is refused with `ESP_ERR_INV_STATE` (0x82) outside WAIT / PLAY or after END. It is refused with `ESP_ERR_INV_SIZE` (0x84) when DATA_SIZE is 0, over 16384 or over FREE. A refused PUSH writes the state registers too, so the host can read FREE and try again.

The DMA of the ESP32-S3 slave takes whole 32-bit words. The host pads the data of a PUSH with zeros to a multiple of 4 bytes. The ESP keeps the DATA_SIZE bytes and drops the padding. A piece of odd length that is not padded stalls the SPI.

START, FLUSH and STOP are done by the decoder task. Their status is BUSY until it has taken them, which is a few ms at most. FREE is 0 until then.

## Flow of a host

```
START (FORMAT = 0 or the format of the file extension)
loop:
    PUSH pieces from the file while FREE allows them (16 KB each, the last one shorter)
    at the end of the file: END
    GET_STATE about once a frame: time = PLAYED / RATE; ENDED or ERROR: done
    keys: PAUSE, VOLUME;
      seek: offset = time * file length / length in seconds,
            FLUSH (DATA_OFFSET = offset), then PUSH from that offset
STOP
```

For a seek, the offset is a byte offset into the file. WAV goes on from the next whole sample frame after it. MP3 and FLAC resync to the next frame header. For an MP3 with an ID3v2 tag the host counts the offset from the end of the tag.

The input buffer is 512 KB in PSRAM: about 13 s of MP3 at 320 kbps, or 2.5 s of FLAC at 44.1 kHz / 16 bit. The decoded samples go through a 64 KB ring of 0.37 s. The decoder task runs on core 1 at priority 8, under `player_task`, and uses about 10 % of the core for FLAC 48 kHz / 24 bit or for MP3 at 320 kbps.

On TS-Conf, Wild Commander reads the SD card at about 270 KB/s (LOAD512, then OTIR to #57). That is enough for everything up to FLAC 48 kHz / 24 bit (about 210 KB/s). FLAC 96 kHz / 24 bit needs about 295 KB/s and runs out of data.

## Tests

`test/audio_host` builds `audio/audio_core.c` (rings, decoders, resampler) on the host with ASan and UBSan. `gen.sh` makes the test files with ffmpeg, and `run_tests.sh` runs the suite. It checks:

- WAV and FLAC bit exact against ffmpeg, MP3 and Ogg within a tolerance;
- pushes in pieces of 1..3000 bytes;
- a flush in the middle of a frame, then playing on;
- garbage input;
- an MP3 with a large ID3v2 tag (cover picture).
