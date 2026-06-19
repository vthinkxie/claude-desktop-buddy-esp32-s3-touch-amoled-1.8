#!/usr/bin/env python3
"""Generate flash_cmd.sh from a `pio run -t upload -v` log.

Usage:
    python tools/generate_flash_cmd.py <env_name> [--port /dev/ttyUSB0]

The script runs PlatformIO upload with a fake port, captures the exact
esptool.py command constructed by PlatformIO, rewrites paths to bare
filenames, copies required helper binaries (e.g. boot_app0.bin) into the
build directory, and writes flash_cmd.sh.
"""

import argparse
import os
import shlex
import shutil
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description="Generate flash_cmd.sh from pio upload output")
    parser.add_argument("env", help="PlatformIO environment name")
    parser.add_argument("--port", default="/dev/ttyUSB0", help="Serial port to use in flash_cmd.sh")
    parser.add_argument("--fake-port", default="/dev/ttyFAKE", help="Fake port used to invoke pio upload")
    args = parser.parse_args()

    build_dir = f".pio/build/{args.env}"

    result = subprocess.run(
        [
            "pio",
            "run",
            "-e",
            args.env,
            "-t",
            "upload",
            "-v",
            "--upload-port",
            args.fake_port,
        ],
        capture_output=True,
        text=True,
    )
    log = result.stdout + result.stderr

    cmd_line = None
    for line in log.splitlines():
        if "esptool.py" in line and "write_flash" in line:
            cmd_line = line.strip()
            break

    if not cmd_line:
        print("=== upload log ===")
        print(log)
        sys.exit("Could not find esptool.py command in pio upload output")

    # The captured line may be wrapped in literal quotes.
    if cmd_line.startswith('"') and cmd_line.endswith('"'):
        cmd_line = cmd_line[1:-1]

    tokens = shlex.split(cmd_line)

    # Drop the Python interpreter path if present.
    if "python" in tokens[0].lower() or tokens[0].endswith("python"):
        tokens = tokens[1:]

    # Use bare `esptool.py` instead of PlatformIO's package path.
    if "esptool.py" in tokens[0]:
        tokens[0] = "esptool.py"

    # Replace serial port.
    for i, t in enumerate(tokens):
        if t == "--port":
            tokens[i + 1] = args.port
            break

    # Rewrite absolute file paths to bare filenames and copy helper binaries
    # (e.g. boot_app0.bin) into the build directory.
    new_tokens = []
    i = 0
    while i < len(tokens):
        new_tokens.append(tokens[i])
        if tokens[i] in ("--chip", "--port", "--baud", "--before", "--after"):
            i += 1
            new_tokens.append(tokens[i])
        elif tokens[i].startswith("0x"):
            i += 1
            src = tokens[i]
            fname = os.path.basename(src)
            dst = os.path.join(build_dir, fname)
            if not os.path.exists(dst) and os.path.exists(src):
                shutil.copy2(src, dst)
            new_tokens.append(fname)
        i += 1

    flash_cmd = " ".join(shlex.quote(t) for t in new_tokens)

    script_path = os.path.join(build_dir, "flash_cmd.sh")
    with open(script_path, "w") as f:
        f.write("#!/bin/bash\n")
        f.write(f"{flash_cmd}\n")
    os.chmod(script_path, 0o755)

    print(f"Wrote {script_path}")
    print(flash_cmd)


if __name__ == "__main__":
    main()
