import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("stage", choices=("checks", "dll"))
stage = parser.parse_args().stage
root = Path(__file__).resolve().parents[1]
env = os.environ.copy()
for key in ("DEPLOY_PATH", "XSE_FO4_MODS_PATH", "XSE_FO4_GAME_PATH"):
    env.pop(key, None)


def checkout(key, pin_file):
    value = env.get(key)
    if not value:
        raise SystemExit(f"Set {key} to the approved dependency checkout")
    path = Path(value).resolve()
    pin = (root / pin_file).read_text(encoding="utf-8").strip()
    if not re.fullmatch(r"[0-9a-f]{40}", pin):
        raise SystemExit(f"Invalid revision pin: {pin_file}")
    git = ["git", "-C", str(path)]
    if subprocess.check_output(git + ["rev-parse", "HEAD"], text=True).strip() != pin:
        raise SystemExit(f"{key} revision mismatch: expected {pin}")
    if subprocess.check_output(git + ["status", "--porcelain", "--ignore-submodules=none"], text=True).strip():
        raise SystemExit(f"{key} checkout has local changes")
    env[key] = str(path)
    return git, pin


if stage == "checks":
    checkout("PRISMA_UI_FRAMEWORK_ROOT", "deps/prisma-ui-framework.ref")
    node = shutil.which("node")
    if not node:
        raise SystemExit("Node.js is required for the app.js syntax check")
    subprocess.run([node, "--check", str(root / "assets/views/PrismaPhotoMode/app.js")], check=True)
    subprocess.run([node, str(root / "tests/letterbox.test.js")], check=True)
    subprocess.run([node, str(root / "tests/ui_polling.test.js")], check=True)
    subprocess.run([sys.executable, "-X", "utf8", "tests/run_checks.py"], cwd=root, env=env, check=True)
    subprocess.run([sys.executable, "-X", "utf8", "tests/test_ci_guards.py"], cwd=root, env=env, check=True)
    print("PrismaPhotoMode asset and runtime policy checks PASS")

elif stage == "dll":
    git, commonlib_sha = checkout("COMMONLIBF4_PATH", "deps/commonlibf4.ref")
    submodules = subprocess.check_output(git + ["submodule", "status", "--recursive", "--", "lib/commonlib-shared"], text=True)
    if any(line[:1] not in ("", " ") for line in submodules.splitlines()):
        raise SystemExit("CommonLibF4 recursive submodules differ from their pins")
    args = ["xmake", "f", "-c", "-p", "windows", "-a", "x64", "-m", "release", "--toolchain=clang-cl[llvm]", "--builddir=build-clang", "-y"]
    if env.get("CLANGCL_SDK"):
        args.append("--sdk=" + env["CLANGCL_SDK"])
    subprocess.run(args, cwd=root, env=env, check=True)
    subprocess.run(["xmake", "build", "-y", "PrismaPhotoMode"], cwd=root, env=env, check=True)
    dll = root / "build-clang/windows/x64/release/PrismaPhotoMode.dll"
    if not dll.is_file():
        raise SystemExit(f"Native build did not produce {dll}")
    manifest = f"commonlib_sha={commonlib_sha}\ndll_sha256={hashlib.sha256(dll.read_bytes()).hexdigest()}\n"
    (dll.parent / "BUILD_PROVENANCE.txt").write_text(manifest + submodules, encoding="utf-8")
    print(manifest, end="")
