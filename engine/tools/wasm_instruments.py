#!/usr/bin/env python3
"""Instrument packages (.odoi) to WebAssembly, for the interpreter experiment.

    tools/wasm_instruments.py PACKAGES_DIR OUT_DIR [name ...]

For each package: OUT_DIR/<name>.wasm and <name>.json (Faust's description: parameter offsets,
size), and <name>.post.wasm / .post.json if it has a `post`. Made with `faust -lang wasm`, as is:
wasm3 leaves its imported memoryBase and tableBase at 0, and mpe::WasmDsp grows the memory to
fit one voice and its buffers.
"""
import glob
import json
import os
import subprocess
import sys
import tempfile
import zipfile


def convert(src_dir, dsp, out_base, process=None):
    cmd = ["faust", "-I", os.path.join(src_dir, "lib"), "-lang", "wasm", "-o", out_base + ".wasm", dsp]
    if process:
        cmd[1:1] = ["-pn", process]
    subprocess.run(cmd, check=True, capture_output=True, cwd=src_dir)


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    pkgs, out = sys.argv[1], os.path.abspath(sys.argv[2])
    names = sys.argv[3:]
    os.makedirs(out, exist_ok=True)
    files = sorted(glob.glob(os.path.join(pkgs, "*.odoi")))
    if names:
        files = [f for f in files if os.path.splitext(os.path.basename(f))[0] in names]
    done = 0
    for f in files:
        name = os.path.splitext(os.path.basename(f))[0]
        with tempfile.TemporaryDirectory() as t:
            zipfile.ZipFile(f).extractall(t)
            man = json.load(open(os.path.join(t, "instrument.json")))
            try:
                convert(t, os.path.join(t, "instrument.dsp"), os.path.join(out, name))
                if man.get("post"):
                    convert(t, os.path.join(t, "instrument.dsp"), os.path.join(out, name + ".post"), "post")
                done += 1
            except subprocess.CalledProcessError as e:
                print(f"{name}: {e.stderr.decode()[:300]}", file=sys.stderr)
    print(f"{done} of {len(files)} instruments as WebAssembly in {out}")


if __name__ == "__main__":
    main()
