import subprocess
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parents[1]
SANDBOX = PROJECT_ROOT / "sandbox"

def test_execute_echo(tmp_path):
    result = subprocess.run(
        [str(SANDBOX), "run", "--", "echo", "Hello"],
        capture_output=True,
        text=True,
        cwd=tmp_path,
        timeout=5,
    )
    assert result.returncode == 0
    assert result.stdout.strip() == "Hello"
