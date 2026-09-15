#!/bin/zsh
# Opens the ATLA editor without the CrashReportClient helper (whose cosmetic
# exit-crashes on macOS 26 look like project crashes). Double-click me.
rm -f "$(dirname "$0")/Saved/Data/PackageRestoreData.json"
exec "/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor.app/Contents/MacOS/UnrealEditor" "$(dirname "$0")/ATLA.uproject" -nocrashreports
