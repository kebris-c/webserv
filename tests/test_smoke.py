#!/usr/bin/env python3
"""External mandatory smoke tests. Run from the repository root."""

import http.client
import concurrent.futures
import os
import signal
import socket
import subprocess
import sys
import time


HOST = "127.0.0.1"
PORT = 8080
UPLOAD = "www/uploads/defense-smoke.txt"
FAIL_CGI = "www/cgi-bin/defense-fail.py"


def request(method, path, body=None, headers=None, port=PORT):
    connection = http.client.HTTPConnection(HOST, port, timeout=3)
    connection.request(method, path, body=body, headers=headers or {})
    response = connection.getresponse()
    data = response.read()
    result = (response.status, dict(response.getheaders()), data)
    connection.close()
    return result


def raw(parts):
    client = socket.create_connection((HOST, PORT), timeout=3)
    client.settimeout(3)
    for part in parts:
        client.sendall(part)
        time.sleep(0.02)
    data = b""
    while True:
        chunk = client.recv(4096)
        if not chunk:
            break
        data += chunk
    client.close()
    return data


def expect(condition, message):
    if not condition:
        raise AssertionError(message)
    print("OK  " + message)


def wait_ready(process):
    deadline = time.time() + 5
    while time.time() < deadline:
        if process.poll() is not None:
            raise RuntimeError("webserv exited during startup")
        try:
            probe = socket.create_connection((HOST, PORT), timeout=0.2)
            probe.close()
            time.sleep(0.05)
            if process.poll() is not None:
                raise RuntimeError(
                    "webserv exited during startup; ports may already be in use")
            return
        except OSError:
            time.sleep(0.05)
    raise RuntimeError("webserv did not listen within 5 seconds")


def run():
    server = subprocess.Popen(
        ["./webserv", "configs/default.conf"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    try:
        wait_ready(server)

        status, _, body = request("GET", "/")
        expect(status == 200 and b"webserv" in body, "static GET")

        status, _, body = request("GET", "/", port=8081)
        expect(status == 200 and body, "second listen port")

        status, headers, _ = request("GET", "/old")
        expect(status == 301 and headers.get("Location") == "/", "redirect")

        status, _, body = request("GET", "/missing")
        expect(status == 404 and b"requested resource was not found" in body,
               "configured error page")

        status, _, body = request("POST", "/")
        expect(status == 405 and b"Method Not Allowed" in body,
               "method not allowed")

        status, _, body = request("GET", "/cgi-bin/")
        expect(status == 403 and b"403 Forbidden" in body,
               "default error page fallback")

        status, _, body = request("GET", "/files/")
        expect(status == 200 and b"readme.txt" in body, "autoindex")

        status, _, body = request("GET", "/cgi-bin/hello.py?defense=yes")
        expect(status == 200 and b"QUERY=defense=yes" in body, "CGI GET")

        with open(FAIL_CGI, "w", encoding="utf-8") as script:
            script.write("import sys\nsys.exit(1)\n")
        status, _, _ = request("GET", "/cgi-bin/defense-fail.py")
        expect(status == 500, "CGI failure")
        os.unlink(FAIL_CGI)

        fragmented = raw([
            b"GET / HTTP/1.1\r\nHo",
            b"st: localhost\r\n\r\n",
        ])
        expect(fragmented.startswith(b"HTTP/1.1 200"), "fragmented request")

        malformed = raw([b"GET / HTTP/1.1\r\n\r\n"])
        expect(malformed.startswith(b"HTTP/1.1 400"), "malformed request")

        chunked = raw([
            b"POST /cgi-bin/hello.py HTTP/1.1\r\n",
            b"Host: localhost\r\nTransfer-Encoding: chunked\r\n\r\n",
            b"5\r\nhello\r\n0\r\n\r\n",
        ])
        expect(chunked.startswith(b"HTTP/1.1 200")
               and b"POST_BODY=hello" in chunked, "chunked CGI POST")

        status, _, _ = request("POST", "/uploads/defense-smoke.txt", b"abc")
        expect(status in (200, 201), "upload_store POST")
        expect(os.path.exists(UPLOAD), "upload written in configured store")

        status, _, body = request("GET", "/uploads/defense-smoke.txt")
        expect(status == 200 and body == b"abc", "uploaded file GET")

        status, _, _ = request("DELETE", "/uploads/defense-smoke.txt")
        expect(status == 204 and not os.path.exists(UPLOAD), "DELETE")

        too_large = raw([
            b"POST /uploads/too-large.txt HTTP/1.1\r\n",
            b"Host: localhost\r\nContent-Length: 2097153\r\n\r\n",
        ])
        expect(too_large.startswith(b"HTTP/1.1 413"), "location body limit")

        dropped = socket.create_connection((HOST, PORT), timeout=3)
        dropped.sendall(b"GET / HTTP/1.1\r\nHost:")
        dropped.close()

        with concurrent.futures.ThreadPoolExecutor(max_workers=20) as pool:
            statuses = list(pool.map(
                lambda _: request("GET", "/")[0], range(100)))
        expect(statuses == [200] * 100, "concurrent availability")

        return_code = server.poll()
        if return_code is not None:
            error_output = server.stderr.read().decode(errors="replace").strip()
            if error_output:
                print(error_output, file=sys.stderr)
        expect(return_code is None,
               "server remains available (exit code %s)" % return_code)
    finally:
        if os.path.exists(UPLOAD):
            os.unlink(UPLOAD)
        if os.path.exists(FAIL_CGI):
            os.unlink(FAIL_CGI)
        if server.poll() is None:
            server.send_signal(signal.SIGINT)
            try:
                server.wait(timeout=3)
            except subprocess.TimeoutExpired:
                server.kill()
                server.wait()


if __name__ == "__main__":
    try:
        run()
    except Exception as error:
        print("FAIL " + str(error), file=sys.stderr)
        sys.exit(1)
