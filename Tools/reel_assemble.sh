#!/bin/zsh
# Assemble the frame dump from reel_choreo.py into the portfolio clip + poster.
# Usage: Tools/reel_assemble.sh [frames_dir] [out_dir] [poster_frame_index]
#
# PIE in the level viewport shares the global screenshot request with the editor's
# other (hidden) level viewports, so a few percent of `Shot` frames come out as the
# editor world (no HUD). Frames are classified by the green health bar at the HUD's
# position and the editor frames are dropped (50 ms skips at 20 fps, invisible).
set -e
PROJECT="$(cd "$(dirname "$0")/.." && pwd)"
FRAMES="${1:-$PROJECT/Saved/Screenshots/MacEditor}"
OUT="${2:-$HOME/dev/portfolio/public/video}"
POSTER_IDX="${3:-290}"
FPS=20
WORK="${TMPDIR:-/tmp}/atla_reel"
mkdir -p "$OUT" "$WORK"
N=$(ls "$FRAMES"/ScreenShot*.png | wc -l | tr -d ' ')
echo "frames: $N"

# 1. one decode pass: average colour of a strip across the health bar (2859x1782 frames)
ffmpeg -v error -framerate $FPS -i "$FRAMES/ScreenShot%05d.png" -vf "crop=150:6:80:1657,scale=1:1" -f rawvideo -pix_fmt rgb24 - > "$WORK/hud.bin"
python3 - "$FRAMES" "$WORK" <<'PY'
import sys
frames, work = sys.argv[1], sys.argv[2]
d = open(work + "/hud.bin", "rb").read(); n = len(d) // 3
good = [i for i in range(n) if d[3*i+1] > 150 and d[3*i+1] > d[3*i] + 40 and d[3*i+1] > d[3*i+2] + 40]
print("game frames: %d, editor frames dropped: %d" % (len(good), n - len(good)))
with open(work + "/list.txt", "w") as f:
    for i in good:
        f.write("file '%s/ScreenShot%05d.png'\nduration %.4f\n" % (frames, i, 1.0 / 20))
PY

# 2. crop the ~16:10 viewport to 16:9 from the bottom (keeps the HUD), 960x540 h264, no audio
ffmpeg -y -v error -f concat -safe 0 -i "$WORK/list.txt" \
  -vf "crop=iw:iw*9/16:0:ih-iw*9/16,scale=960:540:flags=lanczos,format=yuv420p" \
  -r $FPS -c:v libx264 -preset slow -crf 24 -profile:v high -level 4.0 -movflags +faststart -an \
  "$OUT/atla-reel.mp4"
ffmpeg -y -v error -i "$(printf "$FRAMES/ScreenShot%05d.png" "$POSTER_IDX")" \
  -vf "crop=iw:iw*9/16:0:ih-iw*9/16,scale=960:540:flags=lanczos" -q:v 3 "$OUT/atla-reel-poster.jpg"
ls -la "$OUT"
ffprobe -v error -show_entries format=duration:stream=width,height,r_frame_rate -of default=nw=1 "$OUT/atla-reel.mp4"
