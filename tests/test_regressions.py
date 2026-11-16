#!/usr/bin/env python3
"""Regression tests for the HTTP evaluation findings; run from any directory."""

import http.client
import os
from pathlib import Path
import signal
import socket
import subprocess
import tempfile
import time
import unittest


class HttpRegressions(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.repo = Path(__file__).resolve().parents[1]
        cls.temp = tempfile.TemporaryDirectory(prefix="webserv-regressions-")
        cls.root = Path(cls.temp.name)
        for name in ("files/sub dir", "cgi-bin", "post"):
            (cls.root / "www" / name).mkdir(parents=True)
        (cls.root / "www/index.html").write_text("HOME")
        (cls.root / "www/files/sub dir/a b.txt").write_text("SPACE_FILE")
        (cls.root / "www/files/sub dir/a%20b.txt").write_text("LITERAL_PERCENT")
        (cls.root / "www/files/sub dir/a?b#c.txt").write_text("RESERVED")
        (cls.root / "www/cgi-bin/echo.py").write_text(
            "import os,sys\n"
            "body=sys.stdin.buffer.read()\n"
            "sys.stdout.buffer.write(b'Content-Type: text/plain\\n\\n')\n"
            "sys.stdout.buffer.write(os.environ.get('QUERY_STRING','').encode()+b'\\n'+body)\n"
        )
        (cls.root / "www/cgi-bin/loop.py").write_text(
            "import time\nwhile True: time.sleep(.05)\n"
        )
        with socket.socket() as reservation:
            reservation.bind(("127.0.0.1", 0))
            cls.port = reservation.getsockname()[1]
        config = cls.root / "regressions.conf"
        config.write_text("""server {
 listen 127.0.0.1:%d;
 client_max_body_size 1K;
 location / { root www; index index.html; allowed_methods GET; }
 location /files { root www/files; allowed_methods GET; autoindex on; }
 location /post { root www/post; upload_store www/post; allowed_methods POST; }
 location /cgi-bin { root www/cgi-bin; allowed_methods GET POST; cgi_extension .py; cgi_pass /usr/bin/python3; }
}
""" % cls.port)
        cls.log = (cls.root / "server.log").open("w+")
        cls.process = subprocess.Popen(
            [str(cls.repo / "webserv"), str(config)], cwd=cls.root,
            stdout=cls.log, stderr=cls.log,
        )
        try:
            for _ in range(100):
                if cls.process.poll() is not None:
                    cls.log.seek(0)
                    raise RuntimeError(cls.log.read())
                try:
                    with socket.create_connection(("127.0.0.1", cls.port), timeout=.1):
                        return
                except OSError:
                    time.sleep(.03)
            raise TimeoutError("Server did not start")
        except BaseException:
            cls.tearDownClass()
            raise

    @classmethod
    def tearDownClass(cls):
        if cls.process.poll() is None:
            cls.process.send_signal(signal.SIGINT)
            try:
                cls.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                cls.process.kill()
                cls.process.wait()
        cls.log.close()
        cls.temp.cleanup()

    def request(self, method="GET", path="/", body=None):
        connection = http.client.HTTPConnection("127.0.0.1", self.port, timeout=70)
        try:
            connection.request(method, path, body=body)
            response = connection.getresponse()
            return response.status, dict(response.getheaders()), response.read()
        finally:
            connection.close()

    def raw(self, parts):
        with socket.create_connection(("127.0.0.1", self.port), timeout=5) as client:
            client.settimeout(5)
            for part in parts:
                client.sendall(part)
                time.sleep(.002)
            output = bytearray()
            while True:
                block = client.recv(65536)
                if not block:
                    break
                output.extend(block)
        return bytes(output)

    def test_chunked_fragmentation(self):
        headers = (b"POST /cgi-bin/echo.py HTTP/1.1\r\nHost: localhost\r\n"
                   b"Transfer-Encoding: chunked\r\n\r\n")
        cases = [
            [b"5\r", b"\nhe", b"llo\r\n", b"0\r\n", b"\r\n"],
            [b"2\r\nab\r\n", b"3\r\nc", b"de\r", b"\n0\r\n\r\n"],
            [bytes([byte]) for byte in b"5;ext=yes\r\nabcde\r\n0\r\nX-Test: yes\r\n\r\n"],
            [b"0\r\nX-One: 1\r\n", b"X-Two:", b" 2\r\n", b"\r", b"\n"],
        ]
        expected = [b"\nhello", b"\nabcde", b"\nabcde", b"\n"]
        for parts, body in zip(cases, expected):
            with self.subTest(parts=parts):
                response = self.raw([headers] + parts)
                self.assertTrue(response.startswith(b"HTTP/1.1 200 "), response)
                self.assertEqual(response.split(b"\r\n\r\n", 1)[1], body)

    def test_chunked_limits_and_invalid_input(self):
        headers = (b"POST /cgi-bin/echo.py HTTP/1.1\r\nHost: localhost\r\n"
                   b"Transfer-Encoding: chunked\r\n\r\n")
        for body, status in ((b"Z\r\n", 400), (b"1\r\naXX", 400),
                             (b"401\r\n", 413)):
            with self.subTest(body=body):
                response = self.raw([headers + body])
                self.assertTrue(response.startswith(f"HTTP/1.1 {status} ".encode()), response)

    def test_unknown_and_malformed_methods(self):
        self.assertEqual(self.request("UNKNOWN")[0], 501)
        self.assertEqual(self.request("X-CUSTOM")[0], 501)
        response = self.raw([b"GE(T / HTTP/1.1\r\nHost: localhost\r\n\r\n"])
        self.assertTrue(response.startswith(b"HTTP/1.1 400 "), response)
        self.assertEqual(self.request()[0], 200)

    def test_allow_header(self):
        status, headers, _ = self.request("DELETE")
        self.assertEqual(status, 405)
        self.assertEqual(set(headers["Allow"].split(", ")), {"GET", "HEAD"})
        status, headers, _ = self.request("GET", "/post/a.txt")
        self.assertEqual(status, 405)
        self.assertEqual(headers["Allow"], "POST")

    def test_head_has_get_headers_and_no_body(self):
        for path in ("/", "/missing", "/files/", "/cgi-bin/echo.py"):
            with self.subTest(path=path):
                get_status, get_headers, get_body = self.request(path=path)
                status, headers, body = self.request("HEAD", path)
                self.assertEqual(status, get_status)
                self.assertEqual(headers["Content-Length"], get_headers["Content-Length"])
                self.assertEqual(int(headers["Content-Length"]), len(get_body))
                self.assertEqual(body, b"")
        status, headers, body = self.request("HEAD", "/post/a.txt")
        self.assertEqual(status, 405)
        self.assertEqual(headers["Allow"], "POST")
        self.assertEqual(body, b"")

    def test_encoded_paths_and_query(self):
        for path, expected in (
            ("/files/sub%20dir/a%20b.txt", b"SPACE_FILE"),
            ("/files/sub%20dir/a%2520b.txt", b"LITERAL_PERCENT"),
            ("/files/sub%20dir/a%3Fb%23c.txt", b"RESERVED"),
            ("/cgi-bin/echo.py?q=a%20b&x=%2F", b"q=a%20b&x=%2F\n"),
        ):
            with self.subTest(path=path):
                status, _, body = self.request(path=path)
                self.assertEqual(status, 200)
                self.assertEqual(body, expected)
        for path in ("/files/%", "/files/%2", "/files/%GG", "/files/%00.txt"):
            with self.subTest(path=path):
                self.assertEqual(self.request(path=path)[0], 400)
        self.assertEqual(self.request(path="/files/%2e%2e/index.html")[0], 404)

    def test_nested_autoindex_links(self):
        status, _, body = self.request(path="/files/sub%20dir/")
        self.assertEqual(status, 200)
        self.assertIn(b'href="/files/sub%20dir/a%20b.txt"', body)
        self.assertIn(b'href="/files/sub%20dir/a%2520b.txt"', body)
        self.assertIn(b'href="/files/sub%20dir/a%3Fb%23c.txt"', body)

    @unittest.skipUnless(os.environ.get("WEBSERV_TEST_CGI_TIMEOUT") == "1",
                         "Set WEBSERV_TEST_CGI_TIMEOUT=1 for the real 60-second CGI timeout")
    def test_cgi_timeout_returns_504(self):
        start = time.monotonic()
        status, _, body = self.request(path="/cgi-bin/loop.py")
        self.assertEqual(status, 504)
        self.assertIn(b"Gateway Timeout", body)
        self.assertLess(time.monotonic() - start, 70)
        self.assertEqual(self.request()[0], 200)
        children = Path(f"/proc/{self.process.pid}/task/{self.process.pid}/children")
        if children.exists():
            for _ in range(20):
                if not children.read_text().strip():
                    break
                time.sleep(.05)
            self.assertEqual(children.read_text().strip(), "")


if __name__ == "__main__":
    unittest.main(verbosity=2)
