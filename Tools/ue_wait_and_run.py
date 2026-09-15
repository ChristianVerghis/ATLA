#!/usr/bin/env python3
"""Wait for the Unreal Editor's remote execution to come up, then run a script."""
import subprocess
import sys
import time

script = sys.argv[1]
deadline = time.time() + 300
attempt = 0
while time.time() < deadline:
    attempt += 1
    result = subprocess.run(
        [sys.executable, "ue_run.py", "-c", "import unreal; unreal.log('ready')"],
        capture_output=True, text=True,
    )
    if result.returncode == 0:
        print(f"Editor reachable after {attempt} attempts; running {script}")
        final = subprocess.run([sys.executable, "ue_run.py", script])
        sys.exit(final.returncode)
    time.sleep(5)

print("TIMEOUT: editor never became reachable", file=sys.stderr)
sys.exit(1)
