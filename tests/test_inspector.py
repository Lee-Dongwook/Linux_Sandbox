
import subprocess
import time
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
SANDBOX = PROJECT_ROOT / "sandbox"


def test_inspector_reads_current_process():
    process = subprocess.Popen(["sleep", "10"])

    try:
        result = subprocess.run(
            [
                str(SANDBOX),
                "inspect",
                str(process.pid),
            ],
            capture_output=True,
            text=True,
        )

        assert result.returncode == 0
        assert str(process.pid) in result.stdout

    finally:
        process.terminate()
        process.wait()
