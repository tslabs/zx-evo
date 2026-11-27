#!/bin/bash
# gen.sh <dir>: test streams for audio_test (ffmpeg) and their references
# (<name>.ref: ffmpeg's decode, s16le stereo at the source rate)
set -e
D=$1; mkdir -p "$D"; cd "$D"
F="ffmpeg -hide_banner -loglevel error -y"
# 6 s: a sweep on the left, a two-tone chord on the right, a little noise
src() { $F -f lavfi -i "aevalsrc=0.4*sin(2*PI*(200+800*t)*t)|0.3*sin(2*PI*440*t)+0.2*sin(2*PI*1500*t)+0.02*(random(0)-0.5):s=$1:d=6" -ac 2 "$2"; }
src 44100 s44.wav
src 48000 s48.wav
src 22050 s22.wav
$F -i s44.wav -c:a pcm_s16le w16.wav
$F -i s44.wav -c:a pcm_u8 w8.wav
$F -i s48.wav -c:a pcm_s24le w24.wav
$F -i s44.wav -c:a pcm_f32le wf32.wav
$F -i s22.wav -ac 1 -c:a pcm_s16le wmono.wav
$F -i s44.wav -c:a pcm_alaw walaw.wav
$F -i s44.wav -c:a libmp3lame -b:a 192k m44.mp3
$F -i s48.wav -c:a libmp3lame -b:a 128k m48.mp3
$F -i s22.wav -ac 1 -c:a libmp3lame -b:a 64k mmono.mp3
# ID3v2 with a 600 KB picture in front of the audio
$F -f lavfi -i "nullsrc=s=1024x1024,geq=random(1)*255:128:128" -frames:v 1 -c:v png cover.png
$F -i s44.wav -i cover.png -map 0:a -map 1:v -c:a libmp3lame -b:a 192k -id3v2_version 3 -metadata title=Test -disposition:v attached_pic mcover.mp3
# no ID3v2, no Xing frame: the stream starts with an audio frame
$F -i s44.wav -c:a libmp3lame -b:a 160k -id3v2_version 0 -write_xing 0 mraw.mp3
$F -i s44.wav -c:a libvorbis -q:a 5 o44.ogg
$F -i s48.wav -c:a libvorbis -q:a 3 o48.ogg
$F -i s44.wav -c:a flac f16.flac
$F -i s48.wav -c:a flac -sample_fmt s32 -bits_per_raw_sample 24 f24.flac
$F -i s22.wav -ac 1 -c:a flac fmono.flac
for f in w16.wav w8.wav w24.wav wf32.wav wmono.wav walaw.wav m44.mp3 m48.mp3 mmono.mp3 mcover.mp3 mraw.mp3 o44.ogg o48.ogg f16.flac f24.flac fmono.flac; do
  # mono: both channels at full level (-ac 2 would upmix at -3 dB)
  $F -i "$f" -map 0:a -af "pan=stereo|c0=c0|c1=c$(ffprobe -v error -show_entries stream=channels -of csv=p=0 "$f" | head -1 | awk '{print ($1 > 1)}')" -f s16le -c:a pcm_s16le "$f.ref"
done
head -c 300000 /dev/urandom > garbage.bin
rm -f cover.png s44.wav s48.wav s22.wav
