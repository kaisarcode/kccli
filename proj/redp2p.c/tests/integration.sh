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
PHP_BACKEND_PID=""
TCP_PUB_PID=""
UDP_PUB_PID=""
HTTP_PUB_PID=""
TCP_CON_PID=""
UDP_CON_PID=""
HTTP_CON_PID=""

# Stops every child process and removes temporary resources.
# @return None.
cleanup() {
    local pids=(
        "$TCP_CON_PID" "$UDP_CON_PID" "$HTTP_CON_PID"
        "$TCP_PUB_PID" "$UDP_PUB_PID" "$HTTP_PUB_PID"
        "$TCP_BACKEND_PID" "$UDP_BACKEND_PID" "$PHP_BACKEND_PID"
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

# Waits until one TCP listener accepts a connection.
# @param pid Child process id.
# @param port Loopback TCP port.
# @return 0 on success, 1 on timeout or child failure.
wait_tcp_port() {
    local pid="$1"
    local port="$2"

    python3 - "$pid" "$port" <<'PY'
import os
import socket
import sys
import time

pid = int(sys.argv[1])
port = int(sys.argv[2])

for _ in range(200):
    try:
        sock = socket.create_connection(("127.0.0.1", port), timeout=0.1)
        sock.close()
        raise SystemExit(0)
    except OSError:
        try:
            os.kill(pid, 0)
        except OSError:
            raise SystemExit(1)
        time.sleep(0.05)

raise SystemExit(1)
PY
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
command -v php >/dev/null 2>&1 || {
    echo "kccli: php is required for REDP2P HTTP process integration tests" >&2
    exit 1
}
command -v curl >/dev/null 2>&1 || {
    echo "kccli: curl is required for REDP2P HTTP process integration tests" >&2
    exit 1
}
[ -x "$CLI" ] || {
    echo "kccli: REDP2P CLI is not executable: $CLI" >&2
    exit 1
}

TEMP_DIR="$(mktemp -d)"
trap cleanup EXIT INT TERM

read -r INDEX_PORT TCP_BACKEND_PORT TCP_CON_PORT UDP_BACKEND_PORT UDP_CON_PORT PHP_BACKEND_PORT HTTP_CON_PORT <<EOF
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

print(
    tcp_port(),
    tcp_port(),
    dual_port(),
    udp_port(),
    dual_port(),
    tcp_port(),
    dual_port(),
)
PY
)
EOF

cat > "$TEMP_DIR/tcp_backend.py" <<'PY'
import socket
import struct
import sys

port = int(sys.argv[1])

def recv_exact(conn, size):
    data = bytearray()
    while len(data) < size:
        chunk = conn.recv(size - len(data))
        if not chunk:
            return None
        data.extend(chunk)
    return bytes(data)

listener = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listener.bind(("127.0.0.1", port))
listener.listen(16)
print("READY", flush=True)

while True:
    conn, _ = listener.accept()
    try:
        conn.settimeout(10)
        while True:
            header = recv_exact(conn, 4)
            if header is None:
                break
            size = struct.unpack("!I", header)[0]
            if size > 1024 * 1024:
                break
            payload = recv_exact(conn, size)
            if payload is None:
                break
            if payload == b"REDP2P_TEST_CLOSE":
                break
            if payload == b"REDP2P_TEST_HALF_CLOSE":
                if conn.recv(1) != b"":
                    raise RuntimeError("expected client send shutdown")
                response = b"REDP2P_TEST_HALF_CLOSE_OK"
                conn.sendall(struct.pack("!I", len(response)) + response)
                break
            response = bytes((value ^ 0x5a) for value in payload)
            conn.sendall(struct.pack("!I", len(response)) + response)
    except (BrokenPipeError, ConnectionResetError, socket.timeout):
        pass
    finally:
        conn.close()
PY

mkdir -p "$TEMP_DIR/php"
cat > "$TEMP_DIR/php/index.php" <<'PHP'
<?php
header('Content-Type: text/plain');
header('Connection: close');
echo 'TEST123';
PHP

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

php -S "127.0.0.1:$PHP_BACKEND_PORT" -t "$TEMP_DIR/php"     >"$TEMP_DIR/php-backend.log" 2>&1 &
PHP_BACKEND_PID=$!
wait_tcp_port "$PHP_BACKEND_PID" "$PHP_BACKEND_PORT" ||
    fail "PHP backend did not start"

if ! curl --http1.1 --fail --silent --show-error --max-time 10     --dump-header "$TEMP_DIR/http-direct.headers"     --output "$TEMP_DIR/http-direct.body"     "http://127.0.0.1:$PHP_BACKEND_PORT/"     2>"$TEMP_DIR/http-direct.log"
then
    fail "direct PHP HTTP control request failed"
fi
if [ "$(cat "$TEMP_DIR/http-direct.body")" != "TEST123" ]; then
    fail "direct PHP HTTP control body mismatch"
fi

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

run_clean pub "itesthttp@127.0.0.1:$INDEX_PORT" --tcp "$PHP_BACKEND_PORT"     >"$TEMP_DIR/http-pub.log" 2>&1 &
HTTP_PUB_PID=$!
wait_log "$HTTP_PUB_PID" "$TEMP_DIR/http-pub.log"     "redp2p: 'itesthttp' published from local port $PHP_BACKEND_PORT" ||
    fail "HTTP publisher did not register"

run_clean con "itesttcp@127.0.0.1:$INDEX_PORT" "$TCP_CON_PORT"     >"$TEMP_DIR/tcp-con.log" 2>&1 &
TCP_CON_PID=$!
wait_log "$TCP_CON_PID" "$TEMP_DIR/tcp-con.log"     "redp2p: tunnel to 'itesttcp' available on 0.0.0.0:$TCP_CON_PORT" ||
    fail "TCP consumer did not expose its local port"

run_clean con "itestudp@127.0.0.1:$INDEX_PORT" "$UDP_CON_PORT"     >"$TEMP_DIR/udp-con.log" 2>&1 &
UDP_CON_PID=$!
wait_log "$UDP_CON_PID" "$TEMP_DIR/udp-con.log"     "redp2p: tunnel to 'itestudp' available on 0.0.0.0:$UDP_CON_PORT" ||
    fail "UDP consumer did not expose its local port"

run_clean con "itesthttp@127.0.0.1:$INDEX_PORT" "$HTTP_CON_PORT"     >"$TEMP_DIR/http-con.log" 2>&1 &
HTTP_CON_PID=$!
wait_log "$HTTP_CON_PID" "$TEMP_DIR/http-con.log"     "redp2p: tunnel to 'itesthttp' available on 0.0.0.0:$HTTP_CON_PORT" ||
    fail "HTTP consumer did not expose its local port"

if ! python3 - "$TCP_CON_PORT" <<'PY'
import socket
import struct
import sys

port = int(sys.argv[1])
payloads = [
    bytes((i * 17 + 3) & 0xff for i in range(5000)),
    bytes((i * 29 + 11) & 0xff for i in range(17000)),
]

def recv_exact(sock, size):
    data = bytearray()
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise SystemExit(
                f"stream closed after {len(data)} of {size} expected bytes"
            )
        data.extend(chunk)
    return bytes(data)

sock = socket.create_connection(("127.0.0.1", port), timeout=10)
sock.settimeout(15)

for index, payload in enumerate(payloads, 1):
    sock.sendall(struct.pack("!I", len(payload)) + payload)
    response_size = struct.unpack("!I", recv_exact(sock, 4))[0]
    response = recv_exact(sock, response_size)
    expected = bytes((value ^ 0x5a) for value in payload)
    if response != expected:
        raise SystemExit(
            f"stream exchange {index} mismatch: "
            f"{len(response)} bytes received"
        )

close_payload = b"REDP2P_TEST_CLOSE"
sock.sendall(struct.pack("!I", len(close_payload)) + close_payload)
try:
    tail = sock.recv(1)
except socket.timeout:
    raise SystemExit("remote explicit close did not reach the consumer socket")
if tail != b"":
    raise SystemExit(f"unexpected data after remote close: {tail!r}")
sock.close()

sock = socket.create_connection(("127.0.0.1", port), timeout=10)
sock.settimeout(15)
payload = bytes((i * 13 + 5) & 0xff for i in range(7000))
sock.sendall(struct.pack("!I", len(payload)) + payload)
response_size = struct.unpack("!I", recv_exact(sock, 4))[0]
response = recv_exact(sock, response_size)
expected = bytes((value ^ 0x5a) for value in payload)
if response != expected:
    raise SystemExit("new stream after remote close did not roundtrip")
sock.close()

sock = socket.create_connection(("127.0.0.1", port), timeout=10)
sock.settimeout(15)
payload = b"REDP2P_TEST_HALF_CLOSE"
sock.sendall(struct.pack("!I", len(payload)) + payload)
sock.shutdown(socket.SHUT_WR)
response_size = struct.unpack("!I", recv_exact(sock, 4))[0]
response = recv_exact(sock, response_size)
if response != b"REDP2P_TEST_HALF_CLOSE_OK":
    raise SystemExit(f"unexpected response after client shutdown: {response!r}")
if sock.recv(1) != b"":
    raise SystemExit("backend close did not reach half-closed client")
sock.close()
PY
then
    fail "real persistent TCP stream lifecycle failed"
fi

echo "[PASS] real persistent TCP stream lifecycle"

if ! curl --http1.1 --fail --verbose --max-time 15     --dump-header "$TEMP_DIR/http-tunnel.headers"     --output "$TEMP_DIR/http-tunnel.body"     "http://127.0.0.1:$HTTP_CON_PORT/"     2>"$TEMP_DIR/http-tunnel.log"
then
    fail "real PHP HTTP request reset or failed through REDP2P"
fi
if [ "$(cat "$TEMP_DIR/http-tunnel.body")" != "TEST123" ]; then
    fail "real PHP HTTP body mismatch through REDP2P"
fi

echo "[PASS] real PHP HTTP request through REDP2P"

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
echo "3 process integration tests passed"
