#!/bin/zsh
# Launch ATLA as a standalone game window (no editor) — the lean way to
# playtest. Uses far less memory than PIE because the full editor never loads.
# Usage: Tools/play_standalone.sh [extra engine args]
set -e

ENGINE="${UE_ROOT:-/Users/Shared/Epic Games/UE_5.8}/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor"
PROJECT="$(cd "$(dirname "$0")/.." && pwd)/ATLA.uproject"

exec "$ENGINE" "$PROJECT" -game \
  -windowed -resx=1600 -resy=900 \
  -nocrashreports -nosplash \
  "$@"
