#!/bin/bash
# REDP2P CLI process integration test
# Summary: Exercises real idx, pub, and con processes over real sockets.
# Author:  KaisarCode
# Website: https://kaisarcode.com
# License: GNU General Public License v3.0

set -euo pipefail

CLI="${KC_CLI_PATH:-redp2p}"
TEMP_DIR=""
INDEX_PID=""
TCP_BACKEND_PID=""
UDP_BACKEND_PID=""
TCP_PUB_PID=""
UDP_PUB_PID=""
TCP_CON_PID=""
UDP_CON_PID=""

# Stops every child process and removes temporary resources.
# @return None.
cleanup() {
    local pids=(
        "$TCP_CON_PID" "$UDP_CON_PID"
        "$TCP_PUB_PID" "$UDP_PUB_PID"
        "$TCP_BACKEND_PID" "$UDP_BACKEND_PID"
        "$INDEX_PID"
    )
    local pid
    local attempt
    local alive

    for pid in "${pids[@]}"; do
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null || true
        fi
    done

    attempt=0
    while [ "$attempt" -lt 20 ]; do
        alive=0
        for pid in "${pids[@]}"; do
            if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
                alive=1
                break
            fi
        done
        [ "$alive" -eq 0 ] && break
        attempt=$((attempt + 1))
        sleep 0.05
    done

    for pid in "${pids[@]}"; do
        if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
            kill -KILL "$pid" 2>/dev/null || true
        fi
    done
    for pid in "${pids[@]}"; do
        if [ -n "$pid" ]; then
            wait "$pid" 2>/dev/null || true
        fi
    done
    if [ -n "$TEMP_DIR" ]; then
        rm -rf "$TEMP_DIR"
    fi
}

# Prints all process logs before failing.
# @param message Failure summary.
# @return Does not return.
fail() {
    local message="$1"
    local log

    echo "FAIL: $message" >&2
    for log in "$TEMP_DIR"/*.log; do
        [ -f "$log" ] || continue
        echo >&2
        echo "===== $(basename "$log") =====" >&2
        cat "$log" >&2 || true
    done
    exit 1
}

# Waits until one process log contains the requested text.
# @param pid Child process id.
# @param log Log path.
# @param text Expected text.
# @return 0 on success, 1 on timeout or child failure.
wait_log() {
    local pid="$1"
    local log="$2"
    local text="$3"
    local attempt=0

    while [ "$attempt" -lt 200 ]; do
        if grep -Fq "$text" "$log" 2>/dev/null; then
            return 0
        fi
        if ! kill -0 "$pid" 2>/dev/null; then
            return 1
        fi
        attempt=$((attempt + 1))
        sleep 0.05
    done
    return 1
}

# Executes redp2p with operator peer configuration cleared.
# @return Child process exit status.
run_clean() {
    exec env         -u REDP2P_SEATS         -u REDP2P_POW         -u REDP2P_PASS         -u REDP2P_VIP         -u REDP2P_MAX_CONSUMERS_PER_PUBLISHER         -u REDP2P_STUN         -u REDP2P_TURN         -u REDP2P_TURN_USER         -u REDP2P_TURN_PASS         -u REDP2P_FORCE_TURN         "$CLI" "$@"
}

command -v python3 >/dev/null 2>&1 || {
    echo "kccli: python3 is required for REDP2P process integration tests" >&2
    exit 1
}
[ -x "$CLI" ] || {
    echo "kccli: REDP2P CLI is not executable: $CLI" >&2
    exit 1
}

TEMP_DIR="$(mktemp -d)"
trap cleanup EXIT INT TERM

read -r INDEX_PORT TCP_BACKEND_PORT TCP_CON_PORT UDP_BACKEND_PORT UDP_CON_PORT <<EOF
$(python3 - <<'PY'
import socket

held = []

def tcp_port():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("127.0.0.1", 0))
    held.append(s)
    return s.getsockname()[1]

def udp_port():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind(("127.0.0.1", 0))
    held.append(s)
    return s.getsockname()[1]

def dual_port():
    while True:
        t = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        t.bind(("127.0.0.1", 0))
        port = t.getsockname()[1]
        u = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        try:
            u.bind(("127.0.0.1", port))
        except OSError:
            t.close()
            u.close()
            continue
        held.extend((t, u))
        return port

print(tcp_port(), tcp_port(), dual_port(), udp_port(), dual_port())
PY
)
EOF

cat > "$TEMP_DIR/tcp_backend.py" <<'PY'
import socket
import sys

port = int(sys.argv[1])
listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind(("127.0.0.1", port))
listener.listen(16)
print("READY", flush=True)

while True:
    conn, _ = listener.accept()
    try:
        conn.settimeout(5)
        request = bytearray()
        while b"\r\n\r\n" not in request and len(request) < 65536:
            chunk = conn.recv(4096)
            if not chunk:
                break
            request.extend(chunk)
        body = b"TEST123"
        response = (
            b"HTTP/1.1 200 OK\r\n"
            b"Content-Type: text/plain\r\n"
            b"Content-Length: 7\r\n"
            b"Connection: close\r\n"
            b"\r\n" + body
        )
        conn.sendall(response)
    except (BrokenPipeError, ConnectionResetError, socket.timeout):
        pass
    finally:
        conn.close()
PY

cat > "$TEMP_DIR/udp_backend.py" <<'PY'
import socket
import sys

port = int(sys.argv[1])
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("127.0.0.1", port))
print("READY", flush=True)

while True:
    data, peer = sock.recvfrom(65535)
    sock.sendto(data, peer)
PY

python3 -u "$TEMP_DIR/tcp_backend.py" "$TCP_BACKEND_PORT"     >"$TEMP_DIR/tcp-backend.log" 2>&1 &
TCP_BACKEND_PID=$!
wait_log "$TCP_BACKEND_PID" "$TEMP_DIR/tcp-backend.log" "READY" ||
    fail "TCP backend did not start"

python3 -u "$TEMP_DIR/udp_backend.py" "$UDP_BACKEND_PORT"     >"$TEMP_DIR/udp-backend.log" 2>&1 &
UDP_BACKEND_PID=$!
wait_log "$UDP_BACKEND_PID" "$TEMP_DIR/udp-backend.log" "READY" ||
    fail "UDP backend did not start"

run_clean idx "$INDEX_PORT" --pow 0 --max-consumers 32     >"$TEMP_DIR/index.log" 2>&1 &
INDEX_PID=$!
wait_log "$INDEX_PID" "$TEMP_DIR/index.log"     "redp2p: idx listening on port $INDEX_PORT" ||
    fail "index did not become ready"

run_clean pub "itesttcp@127.0.0.1:$INDEX_PORT" --tcp "$TCP_BACKEND_PORT"     >"$TEMP_DIR/tcp-pub.log" 2>&1 &
TCP_PUB_PID=$!
wait_log "$TCP_PUB_PID" "$TEMP_DIR/tcp-pub.log"     "redp2p: 'itesttcp' published from local port $TCP_BACKEND_PORT" ||
    fail "TCP publisher did not register"

run_clean pub "itestudp@127.0.0.1:$INDEX_PORT" --udp "$UDP_BACKEND_PORT"     >"$TEMP_DIR/udp-pub.log" 2>&1 &
UDP_PUB_PID=$!
wait_log "$UDP_PUB_PID" "$TEMP_DIR/udp-pub.log"     "redp2p: 'itestudp' published from local port $UDP_BACKEND_PORT" ||
    fail "UDP publisher did not register"

run_clean con "itesttcp@127.0.0.1:$INDEX_PORT" "$TCP_CON_PORT"     >"$TEMP_DIR/tcp-con.log" 2>&1 &
TCP_CON_PID=$!
wait_log "$TCP_CON_PID" "$TEMP_DIR/tcp-con.log"     "redp2p: tunnel to 'itesttcp' available on 0.0.0.0:$TCP_CON_PORT" ||
    fail "TCP consumer did not expose its local port"

run_clean con "itestudp@127.0.0.1:$INDEX_PORT" "$UDP_CON_PORT"     >"$TEMP_DIR/udp-con.log" 2>&1 &
UDP_CON_PID=$!
wait_log "$UDP_CON_PID" "$TEMP_DIR/udp-con.log"     "redp2p: tunnel to 'itestudp' available on 0.0.0.0:$UDP_CON_PORT" ||
    fail "UDP consumer did not expose its local port"

if ! python3 - "$TCP_CON_PORT" <<'PY'
import socket
import sys

port = int(sys.argv[1])
sock = socket.create_connection(("127.0.0.1", port), timeout=10)
sock.settimeout(15)
sock.sendall(
    b"GET / HTTP/1.1\r\n"
    b"Host: redp2p.test\r\n"
    b"Connection: close\r\n"
    b"\r\n"
)

data = bytearray()
header_end = -1
content_length = None
while True:
    chunk = sock.recv(4096)
    if not chunk:
        break
    data.extend(chunk)
    if header_end < 0:
        header_end = data.find(b"\r\n\r\n")
        if header_end >= 0:
            header = bytes(data[:header_end]).decode("latin1")
            for line in header.split("\r\n"):
                if line.lower().startswith("content-length:"):
                    content_length = int(line.split(":", 1)[1].strip())
                    break
    if header_end >= 0 and content_length is not None:
        body_start = header_end + 4
        if len(data) - body_start >= content_length:
            break

sock.close()
if header_end < 0:
    raise SystemExit("HTTP response headers were not received")
status = bytes(data[:header_end]).split(b"\r\n", 1)[0]
body = bytes(data[header_end + 4:])
if status != b"HTTP/1.1 200 OK":
    raise SystemExit(f"unexpected HTTP status: {status!r}")
if content_length != 7:
    raise SystemExit(f"unexpected content length: {content_length!r}")
if body[:7] != b"TEST123":
    raise SystemExit(f"unexpected HTTP body: {body!r}")
PY
then
    fail "real TCP/HTTP roundtrip failed through idx -> con -> pub -> backend"
fi

echo "[PASS] real TCP/HTTP process roundtrip"

if ! python3 - "$UDP_CON_PORT" <<'PY'
import socket
import sys

port = int(sys.argv[1])
payload = b"redp2p-real-udp-roundtrip"
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.settimeout(15)
sock.sendto(payload, ("127.0.0.1", port))
data, _ = sock.recvfrom(65535)
sock.close()
if data != payload:
    raise SystemExit(f"unexpected UDP payload: {data!r}")
PY
then
    fail "real UDP roundtrip failed through idx -> con -> pub -> backend"
fi

echo "[PASS] real UDP process roundtrip"
echo
echo "2 process integration tests passed"
