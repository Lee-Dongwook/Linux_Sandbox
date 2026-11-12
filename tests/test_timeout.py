
import subprocess
import time
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SANDBOX = PROJECT_ROOT / "sandbox"


def test_process_timeout(tmp_path):
    start = time.monotonic()

    result = subprocess.run(
        [
            str(SANDBOX),
            "run",
            "--timeout",
            "2",
            "--",
            "sleep",
            "10",
        ],
        capture_output=True,
        text=True,
        cwd=tmp_path,
        timeout=5,
    )

    elapsed = time.monotonic() - start

    assert result.returncode == 124
    assert 1.5 <= elapsed < 5


def test_timeout_kills_child_process_group(tmp_path):
    result = subprocess.run(
        [
            str(SANDBOX),
            "run",
            "--timeout",
            "2",
            "--",
            "sh",
            "-c",
            "sleep 100 & wait",
        ],
        capture_output=True,
        text=True,
        cwd=tmp_path,
        timeout=5,
    )

    assert result.returncode == 124

    # The shell and its child should no longer be running.
    check = subprocess.run(
        ["pgrep", "-f", "^sleep 100$"],
        capture_output=True,
        text=True,
        timeout=2,
    )

    assert check.returncode == 1
