import json
import os
import socket
import subprocess
import time
from pathlib import Path

import pytest

PROJECT_ROOT = Path(__file__).resolve().parents[1]
SANDBOX = PROJECT_ROOT / "sandbox"
SOCKET_PATH = "/tmp/mini-sandbox.sock"

# system.metrics relies on the /proc-based System Inspector (Linux only).
pytestmark = pytest.mark.skipif(
    not os.path.exists("/proc/stat"), reason="IPC server requires Linux /proc"
)


@pytest.fixture(scope="module")
def server():
    proc = subprocess.Popen([str(SANDBOX), "--server"],
                            stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
    for _ in range(50):
        if os.path.exists(SOCKET_PATH):
            break
        time.sleep(0.1)
    else:
        proc.kill()
        pytest.fail("server did not create socket: " + proc.stderr.read().decode())
    yield proc
    proc.kill()
    proc.wait()


def send_raw(data):
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.settimeout(5)
        client.connect(SOCKET_PATH)
        client.sendall(data)
        response = b""
        while not response.endswith(b"\n"):
            chunk = client.recv(4096)
            if not chunk:
                break
            response += chunk
    return json.loads(response.decode())


def request(payload):
    return send_raw((json.dumps(payload) + "\n").encode())


def test_system_metrics(server):
    result = request({"version": 1, "request_id": 1, "command": "system.metrics"})
    assert result["status"] == "ok"
    assert result["request_id"] == 1
    assert result["data"]["cpu_count"] > 0
    assert result["data"]["memory_available_kb"] > 0


def test_unknown_command(server):
    result = request({"version": 1, "request_id": 2, "command": "process.unknown"})
    assert result["error"]["code"] == "UNKNOWN_COMMAND"
    assert result["request_id"] == 2


def test_unsupported_version(server):
    result = request({"version": 2, "request_id": 3, "command": "system.metrics"})
    assert result["error"]["code"] == "UNSUPPORTED_VERSION"


def test_invalid_json(server):
    result = send_raw(b'{"version":1,"request_id":4,\n')
    assert result["error"]["code"] == "INVALID_JSON"


def test_client_disconnect_does_not_kill_server(server):
    # Close before reading the response: the server's write hits EPIPE.
    for _ in range(20):
        with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
            client.connect(SOCKET_PATH)
            client.sendall(b'{"version":1,"request_id":5,"command":"system.metrics"}\n')
    time.sleep(0.2)
    assert server.poll() is None, "server died after client disconnect"
    result = request({"version": 1, "request_id": 6, "command": "system.metrics"})
    assert result["status"] == "ok"
