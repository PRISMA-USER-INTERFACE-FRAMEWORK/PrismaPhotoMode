from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
checker = root / "tests/static_checks.py"
with tempfile.TemporaryDirectory(prefix="prisma-photo-mode-ci-") as tmp:
    fixture = Path(tmp)
    for directory in ("assets", "src"):
        shutil.copytree(root / directory, fixture / directory)
    index = fixture / "assets/views/PrismaPhotoMode/index.html"
    original = index.read_bytes()
    cases = (
        ("duplicate id", original + b'<div id="panel"></div>', "duplicate DOM ids"),
        ("missing control", original.replace(b'id="screenshot-btn"', b'id="missingShot"'), "missing static control"),
    )
    for name, blob, marker in cases:
        index.write_bytes(blob)
        result = subprocess.run(
            [sys.executable, "-X", "utf8", str(checker), str(fixture)],
            capture_output=True,
            text=True,
        )
        assert result.returncode and marker in result.stdout + result.stderr, (name, result.stdout, result.stderr)
print("PrismaPhotoMode static guard negative fixtures PASS")
