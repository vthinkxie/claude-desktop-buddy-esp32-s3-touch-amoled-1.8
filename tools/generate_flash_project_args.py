#!/usr/bin/env python3
"""Generate flash_project_args from a `pio run -t upload -v` log.

The output file follows the ESP-IDF flash_args style and can be used with:

    esptool.py -c <chip> -p <port> -b <baud> @flash_project_args

Usage:
    python tools/generate_flash_project_args.py <env_name>
"""

import argparse
import os
import shlex
import shutil
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description="Generate flash_project_args from pio upload output")
    parser.add_argument("env", help="PlatformIO environment name")
    parser.add_argument(
        "--fake-port", default="/dev/ttyFAKE", help="Fake port used to invoke pio upload"
    )
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

    if cmd_line.startswith('"') and cmd_line.endswith('"'):
        cmd_line = cmd_line[1:-1]

    tokens = shlex.split(cmd_line)

    # Drop leading Python interpreter path if present.
    if "python" in tokens[0].lower() or tokens[0].endswith("python"):
        tokens = tokens[1:]

    # We are only interested in the arguments after write_flash.
    try:
        wf_idx = tokens.index("write_flash")
    except ValueError:
        sys.exit("Could not find 'write_flash' in esptool command")

    args_after_write_flash = tokens[wf_idx + 1 :]

    # Extract flash options and file/offset pairs.
    flash_options = []
    file_pairs = []
    i = 0
    while i < len(args_after_write_flash):
        tok = args_after_write_flash[i]
        if tok in ("-z", "--compress"):
            # -z is the default compressed write; omit it from the args file
            # to keep the file IDF-flash_args style.
            i += 1
        elif tok in ("--flash_mode", "--flash_freq", "--flash_size"):
            flash_options.append(tok)
            i += 1
            flash_options.append(args_after_write_flash[i])
            i += 1
        elif tok.startswith("0x"):
            offset = tok
            i += 1
            src = args_after_write_flash[i]
            fname = os.path.basename(src)
            dst = os.path.join(build_dir, fname)
            if not os.path.exists(dst) and os.path.exists(src):
                shutil.copy2(src, dst)
            file_pairs.append((offset, fname))
            i += 1
        else:
            # Pass through anything else (e.g. future flags).
            flash_options.append(tok)
            i += 1

    out_path = os.path.join(build_dir, "flash_project_args")
    with open(out_path, "w") as f:
        for opt in flash_options:
            f.write(f"{opt}\n")
        for offset, fname in file_pairs:
            f.write(f"{offset} {fname}\n")

    print(f"Wrote {out_path}")
    with open(out_path) as f:
        print(f.read())


if __name__ == "__main__":
    main()
