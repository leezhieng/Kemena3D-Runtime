#!/usr/bin/env python3
# Kemena3D runtime ("export template") builder.
#
# Builds the generic player executable natively on the current OS, against the
# already-built Kemena3D SDK. Run this once per platform; the resulting binary
# in bin/<Config>/ is the export template the editor copies when exporting a game.
#
# Prereqs on this machine: deps built (download_dep.py) and SDK built (build_sdk.py).

import os
import platform
import subprocess
import sys
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
DEFAULT_SDK_DIR = str(SCRIPT_DIR.parent / "kemena3d" / "Output")


def run_cmd(cmd, cwd=None):
    print(f"\n> {cmd}")
    result = subprocess.run(cmd, shell=True, cwd=cwd)
    if result.returncode != 0:
        sys.exit(result.returncode)


def choose(prompt, options, env=None):
    # Non-interactive override for CI.
    if env:
        val = os.environ.get(env, "").strip()
        if val in options:
            print(f"{prompt}\n  [{env}={val}] (non-interactive)")
            return val
    print(prompt)
    for k, v in options.items():
        print(f"  {k}. {v}")
    c = input("Enter your choice: ").strip()
    return c if c in options else next(iter(options))


def main():
    system = platform.system()
    print(f"=== Kemena3D Runtime Builder ({system}) ===")

    # Compiler / generator per platform (mirrors build_editor.py).
    if system == "Windows":
        compiler = choose("\nChoose a toolchain:",
                          {"1": "Visual Studio 2022", "2": "MinGW (GCC)"},
                          env="KEMENA_COMPILER")
        if compiler == "1":
            generator = "Visual Studio 17 2022"
            extra = "-DUSE_MINGW=OFF"
            make_program = ""
        else:
            generator = "MinGW Makefiles"
            extra = "-DUSE_MINGW=ON"
            make_program = ' -DCMAKE_MAKE_PROGRAM=mingw32-make'
    elif system in ("Linux", "FreeBSD"):
        generator = "Unix Makefiles"
        extra = ""
        make_program = ""
    elif system == "Darwin":
        generator = "Xcode"
        extra = ""
        make_program = ""
    else:
        print(f"Unsupported platform: {system}")
        sys.exit(1)

    cfg = choose("\nChoose a configuration:",
                 {"1": "Debug", "2": "Release", "3": "Both"},
                 env="KEMENA_CONFIG")
    modes = {"1": ["Debug"], "2": ["Release"], "3": ["Debug", "Release"]}[cfg]

    sdk_env = os.environ.get("KEMENA_SDK_DIR", "").strip()
    if sdk_env:
        sdk_dir = sdk_env
    else:
        sdk_input = input(f"\nKemena3D SDK directory [{DEFAULT_SDK_DIR}]: ").strip()
        sdk_dir = sdk_input if sdk_input else DEFAULT_SDK_DIR

    # Validate the SDK path so a typo doesn't surface as a confusing
    # "cannot open kemena/kemena.h" compile error later.
    sdk = Path(sdk_dir)
    if not ((sdk / "Debug" / "include" / "kemena" / "kemena.h").exists() or
            (sdk / "Release" / "include" / "kemena" / "kemena.h").exists()):
        print(f"\n[ERROR] '{sdk_dir}' is not a built Kemena3D SDK "
              f"(no Debug|Release/include/kemena/kemena.h).")
        print("        Build it first with kemena3d/build_sdk.py, then pass its "
              "Output folder (e.g. D:/Projects/Kemena3D/kemena3d/Output).")
        sys.exit(1)

    for mode in modes:
        build_dir = SCRIPT_DIR / f"build_{mode}"
        run_cmd(
            f'cmake -S "{SCRIPT_DIR}" -B "{build_dir}" -G "{generator}" '
            f'-DCMAKE_BUILD_TYPE={mode} '
            f'-DKEMENA3D_SDK_DIR="{sdk_dir}" '
            f'-DKEMENA3D_LINK_STATIC=ON {extra}{make_program}'
        )
        run_cmd(f'cmake --build "{build_dir}" --config {mode} --parallel')

    print("\n[SUCCESS] Runtime template(s) built into bin/. "
          "These are the export templates the editor bundles.")


if __name__ == "__main__":
    main()
