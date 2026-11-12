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

def test_execute_nonzero_exit_code(tmp_path):
    result = subprocess.run(
        [str(SANDBOX), "run", "--", "false"],
        capture_output=True,
        text=True,
        cwd=tmp_path,
        timeout=5,
    )

    assert result.returncode == 1

def test_execute_command_not_found(tmp_path):
    result = subprocess.run(
        [str(SANDBOX), "run", "--", "mini-sandbox-command-that-does-not-exist"],
        capture_output=True,
        text=True,
        cwd=tmp_path,
        timeout=5,
    )

    assert result.returncode == 127
    assert "execvp" in result.stderr


def test_exec_error_is_reported(tmp_path):
    result = subprocess.run(
        [
            str(SANDBOX),
            "run",
            "--",
            "mini-sandbox-command-that-does-not-exist",
        ],
        capture_output=True,
        text=True,
        cwd=tmp_path,
        timeout=5,
    )

    assert result.returncode == 127
    assert "execvp" in result.stderr
    assert "No such file or directory" in result.stderr


def test_program_exit_127_is_not_exec_error(tmp_path):
    result = subprocess.run(
        [
            str(SANDBOX),
            "run",
            "--",
            "sh",
            "-c",
            "exit 127",
        ],
        capture_output=True,
        text=True,
        cwd=tmp_path,
        timeout=5,
    )

    assert result.returncode == 127
    assert "execvp" not in result.stderr
