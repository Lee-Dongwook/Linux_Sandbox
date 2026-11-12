import signal
import subprocess
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SANDBOX = PROJECT_ROOT / "sandbox"


def test_cpu_limit(tmp_path):
    result = subprocess.run(
        [
            str(SANDBOX),
            "run",
            "--cpu",
            "1",
            "--",
            "python3",
            "-c",
            "while True: pass",
        ],
        capture_output=True,
        text=True,
        cwd=tmp_path,
        timeout=5,
    )

    assert result.returncode == 128 + signal.SIGXCPU
    assert "Child terminated by signal" in result.stderr


def test_memory_limit(tmp_path):
    result = subprocess.run(
        [
            str(SANDBOX),
            "run",
            "--memory",
            "64",
            "--",
            "python3",
            "-c",
            """
data = bytearray(128 * 1024 * 1024)
print("allocation succeeded")
""",
        ],
        capture_output=True,
        text=True,
        cwd=tmp_path,
        timeout=5,
    )

    assert result.returncode != 0
    assert "allocation succeeded" not in result.stdout
