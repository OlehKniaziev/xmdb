import hashlib
import os
import socket
import subprocess
import tempfile
import time
import hashlib


def sha256_hex(password):
    return hashlib.sha256(password.encode("utf-8")).hexdigest()

def find_free_port():
    """Bind to port 0, get the assigned port, then release the socket."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind(("127.0.0.1", 0))
        return s.getsockname()[1]


def wait_for_server(port, timeout=10.0, interval=0.2):
    """Block until a TCP connection to localhost:port succeeds or timeout expires."""
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=1.0):
                return True
        except OSError:
            time.sleep(interval)
    raise RuntimeError(f"Server on port {port} did not become ready within {timeout}s")


def start_server(port):
    """Start xmdb_server in a temporary directory and wait for it to be ready.

    Returns (process, tmpdir_path).
    """
    server_bin = os.environ.get("SERVER_BIN")
    if not server_bin:
        raise RuntimeError("SERVER_BIN environment variable not set")

    tmpdir = tempfile.mkdtemp(prefix="xmdb_integration_")

    proc = subprocess.Popen(
        [server_bin, "-port", str(port)],
        cwd=tmpdir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    try:
        wait_for_server(port)
    except RuntimeError:
        proc.kill()
        proc.wait()
        raise

    return proc, tmpdir
