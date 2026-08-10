#!/usr/bin/env python3
# OWNER: kmarrero (script + CGI HTTP contract) / kebris-c (server executes it)
# LEARN: CGI prints headers, blank line, then body on stdout.
# Server must set env vars and feed POST body on stdin.

import os
import sys

body = "Hello from webserv CGI\n"
method = os.environ.get("REQUEST_METHOD", "")
query = os.environ.get("QUERY_STRING", "")
body += "METHOD=" + method + "\n"
body += "QUERY=" + query + "\n"
if method == "POST":
    length = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
    payload = sys.stdin.read(length) if length > 0 else ""
    body += "POST_BODY=" + payload + "\n"

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("Content-Length: " + str(len(body)) + "\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(body)
