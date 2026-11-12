
import json
import subprocess
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[1]
SANDBOX = PROJECT_ROOT / "sandbox"


def test_execution_log(tmp_path):
    result = subprocess.run(
        [str(SANDBOX), "run", "--", "echo", "Hello"],
        capture_output=True,
        text=True,
        cwd=tmp_path,
        timeout=5,
    )

    assert result.returncode == 0

    log_file = tmp_path / "sandbox.jsonl"

    assert log_file.exists()

    lines = log_file.read_text().splitlines()

    assert len(lines) == 1

    log = json.loads(lines[0])

    assert log["command"] == ["echo", "Hello"]
    assert log["exit_code"] == 0
    assert log["timed_out"] is False
    assert log["duration_ms"] >= 0
