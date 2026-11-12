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


def test_file_size_limit(tmp_path):
    output_file = tmp_path / "large.bin"

    result = subprocess.run(
        [
            str(SANDBOX),
            "run",
            "--file-size",
            "1",
            "--",
            "python3",
            "-c",
            """
with open("large.bin", "wb") as f:
    f.write(b"x" * (2 * 1024 * 1024))
""",
        ],
        capture_output=True,
        text=True,
        cwd=tmp_path,
        timeout=5,
    )

    print("returncode:", result.returncode)
    print("stdout:", result.stdout)
    print("stderr:", result.stderr)
    print("file exists:", output_file.exists())

    if output_file.exists():
        print("file size:", output_file.stat().st_size)

    assert result.returncode != 0
    assert output_file.exists()
    assert output_file.stat().st_size <= 1024 * 1024
