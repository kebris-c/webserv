# kmarrero — full walkthrough (HTTP + config + content plane)

> **INTERNAL — do not include in the evaluation Git submit.**
> Local study / pair communication only. Listed in `.gitignore` under team docs.
> Official public README is the root [`README.md`](README.md).

Scrapers and web tooling help you with headers, status codes, forms, and configs. They do **not** replace learning HTTP framing. If `Request::parse` is wrong, kebris-c’s poll loop will look guilty when the bug is yours.

**Server language: C++98 only.** Your code lives in `src/*.cpp`. Python is allowed only as CGI scripts (`www/cgi-bin/`) and external tests (`tests/`). Rules: [`SUBJECT_RULES.md`](SUBJECT_RULES.md).

Partner guide: [`KEBRIS-C.md`](KEBRIS-C.md) · Shared phases: [`docs/WORK_SPLIT.md`](docs/WORK_SPLIT.md) · Seam ledger: [`docs/README.md`](docs/README.md)

---

## Integration status (read first — Aug 2026)

kebris-c’s transport/`poll`/CGI process plane is already wired and waiting on **your**
HTTP/config plane. Until you implement the stubs below, the binary only proves TCP
echo — **not** subject HTTP.

### Forced / temporary touchpoints you must review

Search for these markers before rewriting behaviour:

| Where | What happened | Your action |
|---|---|---|
| `src/main.cpp` | If `Config::load` fails, kebris-c workaround binds `8080`/`8081` and enables TCP echo | Make `load()` succeed; then **delete** the echo fallback (invalid config must fail) |
| `src/Server.cpp` | Echo branch copies `readBuf` → `writeBuf` when that mode is on | Do not treat echo as HTTP; it disappears when config works |
| `HttpHandler::prepareCgi` / `parseCgiOutput` | Public API stubs **added** so Server can call CGI without owning HTTP semantics; both currently return “no CGI” / `501` | Replace with real route/env decision and CGI-header→HTTP conversion |
| `CgiProcess::buildEnv` | Still your stub; kebris-c already executes `start`/pipes/`waitpid(WNOHANG)` | Fill RFC 3875 `KEY=VALUE` strings |
| `LocationConfig` | No per-location body-size field yet, but `configs/default.conf` has `client_max_body_size` inside `/upload` | Extend the struct **or** change the sample conf — keep them consistent |
| `REMOTE_ADDR` | `Connection::remoteAddr()` already filled at accept; passed into `prepareCgi` | Put `REMOTE_ADDR=<remoteAddr>` in the env vector (and/or `buildEnv`) |

Workaround banner used by kebris-c (do not leave in final code):

```text
// TODO: This is a workaround to allow me keep going on, must be
// changed/improved before the project end.
```

Also keep existing `TODO(kmarrero):` comments in your owned `.cpp` files — they mark
real unfinished mandatory work, not optional polish.

### Contract Server already calls (do not rename casually)

```text
Config::load(path) -> servers()
Request::parse(readBuf)   # incremental; must consume bytes
Router::match(server, request)
HttpHandler::prepareCgi(...) -> false | scriptPath + env  # + remoteAddr arg
HttpHandler::handle(...) -> Response
HttpHandler::parseCgiOutput(cgiStdout) -> Response
Response::raw() / Response::makeError(...)
CgiProcess::buildEnv(...)   # called from your prepareCgi path; set REMOTE_ADDR
```

Hard rule unchanged: **no** `recv`/`send`/`read`/`write` on sockets/pipes in your code.

Full seam/workaround table: [`docs/README.md`](docs/README.md).

### Stepped-on APIs — location, why, what you do, what you must respect

kebris-c left **explicit TODO banners** on every forced touchpoint. Expand them in
the `.cpp` if anything is unclear. Summary:

| Location | Why kebris-c touched it | What you implement | Must keep / must not break |
|---|---|---|---|
| `src/main.cpp` `else` after failed `load` | Without listeners, I/O plane cannot run | Real `Config::load`; **delete** fallback + echo flag | CLI `./webserv [conf]`; `configure` + `run` only; invalid conf → error exit |
| `Server::_onReadable` echo branch | Transport demo while HTTP stubs exist | Ensure echo is never enabled; **delete** branch | No `recv`/`send` in your code; leave poll readiness, partial send, timeouts, CGI wiring |
| `HttpHandler::prepareCgi` (+ header decl) | Server needs CGI yes/no + path/env without owning HTTP | Return `true` + `scriptPath` + env (incl. `REMOTE_ADDR=`) or `false` → `handle` | **Keep signature** (incl. `remoteAddr`); no fork/pipe/poll; body already unchunked |
| `HttpHandler::parseCgiOutput` | Server has raw stdout after async CGI; needs `Response` | Parse CGI headers/`Status`, body; build `Response` | No socket/pipe I/O; input is full `CgiProcess::output()` after reap |
| `CgiProcess::buildEnv` | Process/pipes already done by kebris-c | Fill `KEY=VALUE` vector; call from `prepareCgi` | Do not change `start`/pipe ownership/`waitpid(WNOHANG)` paths |
| `Connection::remoteAddr()` | Peer captured at `accept` (whitelist-safe IPv4 text) | Put into env as `REMOTE_ADDR=` | Do not remove getter; do not require `inet_ntop` |

Also: `configs/default.conf` has per-location `client_max_body_size` but
`LocationConfig` does not — align struct **or** conf before claiming parser done.

Search:

```bash
rg 'TODO: This is a workaround' src include
rg 'WHY \\(kebris-c\\)|YOU \\(kmarrero\\)|RESPECT:' src
```

---

## 0. Mindset

You own the **meaning** of bytes. kebris-c owns **moving** bytes safely.

| You build | You do **not** build |
|---|---|
| Config → structs | `poll` loop |
| Request/Response | `recv`/`send` |
| Router + static/upload/DELETE | `fork`/`pipe` mechanics |
| CGI env + CGI stdout→HTTP | registering pipe fds in poll |
| Demo site + tests + eval configs | stress internals of the loop (help with scripts) |

Defense still requires you to explain why socket I/O must wait for poll.

---

## 1. Owned files

| Path | Job |
|---|---|
| `Config.*` + `configs/*` | nginx-inspired parser + eval configs |
| `Request.*` | incremental parser (incl. chunked unchunk) |
| `Response.*` | status/headers/body → wire string |
| `Router.*` | longest-prefix location + URI→fs path |
| `HttpHandler.*` | GET/POST/DELETE/redirect/autoindex/upload/CGI dispatch |
| `CgiProcess::buildEnv` + CGI output parse | CGI contract |
| `www/**` | static demo, errors, uploads dir, CGI scripts |
| `tests/**` | Python (or other) external tests |
| README accuracy | keep Instructions/Resources honest |

---

## 2. Non-negotiable clarifications

1. **No `recv`/`send` in your handlers.** Buffers only.  
2. **Incremental parse.** Bytes arrive in pieces; resume across calls.  
3. **Unchunk before CGI.** Subject requirement.  
4. **Multi-port required; virtual hosts optional** (out of scope).  
5. **Status codes must be accurate.** Build a table; use it everywhere.  
6. **Config is product.** Eval demos run on *your* `.conf` files.  
7. **nginx is the oracle** for ambiguous behaviour.  
8. **C++98:** no ranged-for, no `auto` as C++11, no `std::to_string`, no `unordered_map` unless you are sure — prefer `map`/`vector`/`stringstream`.  
9. **Path traversal is your bug** even if kebris-c serves the file. Reject `..` and nasty encodings.  
10. Freeze structs/API with kebris-c before changing phase-3 wiring ([`docs/WORK_SPLIT.md`](docs/WORK_SPLIT.md), [`KEBRIS-C.md`](KEBRIS-C.md)). `prepareCgi` / `parseCgiOutput` are already frozen call sites.

---

## 3. What to study (ordered)

### Paper before code

Write by hand (or in a text file) these requests and mark where headers end / body ends:

```http
GET / HTTP/1.1
Host: localhost

```

```http
POST /upload HTTP/1.1
Host: localhost
Content-Length: 5

hello
```

```http
POST /cgi-bin/hello.py HTTP/1.1
Host: localhost
Transfer-Encoding: chunked

5\r\nhello\r\n0\r\n\r\n
```

If you cannot find the body boundary on paper, the parser will hang the browser.

### References

| Resource | Use |
|---|---|
| [RFC 9112](https://www.rfc-editor.org/rfc/rfc9112.html) | request line, headers, chunked |
| [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110.html) | method/status semantics |
| [RFC 3875](https://datatracker.ietf.org/doc/html/rfc3875) | CGI env + response |
| nginx docs (`server`, `location`, `root`, `index`, `error_page`, `client_max_body_size`) | config model |
| MDN: HTTP status / CORS not needed / forms | upload mental model |
| `man 3 opendir` `readdir` `stat` | autoindex + file types |

### Observe nginx

Run a minimal nginx with similar roots and compare:

- status line for missing file  
- redirect responses (`Location`)  
- method not allowed  
- directory with/without index and autoindex  

```bash
curl -v http://127.0.0.1:8080/missing
curl -v -X POST http://127.0.0.1:8080/
curl -v -X DELETE http://127.0.0.1:8080/files/readme.txt
```

---

## 4. Config design (phase 1 deep dive)

### Target grammar (keep small)

Your `configs/default.conf` already sketches directives. Support at least:

```text
server {
  listen HOST:PORT;          # or PORT with default host
  server_name NAME;          # optional
  client_max_body_size SIZE; # e.g. 10M
  error_page CODE PATH;

  location /path {
    root DIR;
    index FILE;
    autoindex on|off;
    allowed_methods GET POST DELETE;
    return CODE URL;         # redirect
    upload_store DIR;
    cgi_extension .py;
    cgi_pass /usr/bin/python3;
  }
}
```

You may choose equivalent directive names if documented — but closer to nginx = less eval confusion.

### Parser strategy (recommended)

1. Read whole file (config is small).  
2. Strip `#` comments.  
3. Tokenize: `{` `}` `;` and words.  
4. Recursive-descent or brace-counting for `server` / `location` blocks.  
5. Fill `ServerConfig` / `LocationConfig`.  
6. Validate: at least one listen, locations non-empty ideally, numeric ports, known sizes.

### Size parsing

`10M` → `10 * 1024 * 1024` (document whether you use 1000 or 1024; be consistent).

### Deliverable for kebris-c

`Config::load` succeeds on `minimal.conf` and `default.conf`; `servers()` returns listen host/port they can bind.

**Unit dump:** print configs to stderr in debug builds to verify nesting.

### Subject mapping example

> URL `/kapouet` rooted to `/tmp/www` →  
> `/kapouet/pouic/toto/pouet` searches `/tmp/www/pouic/toto/pouet`

Implement this join carefully in `Router` (strip location prefix, append to root, normalize slashes, reject `..`).

---

## 5. Request parser (phase 2 deep dive)

### States

```text
REQ_LINE -> REQ_HEADERS -> REQ_BODY -> REQ_COMPLETE
                              \-> REQ_ERROR
```

### Algorithm

```text
parse(buffer):
  loop:
    if REQ_LINE:
      need line ending with \r\n
      split METHOD SP REQUEST_TARGET SP HTTP/VERSION
      split target into path and query at first '?'
      -> REQ_HEADERS
    if REQ_HEADERS:
      read lines until empty line \r\n\r\n
      store headers (normalize names to lowercase internally)
      detect Content-Length / Transfer-Encoding: chunked
      if no body expected -> COMPLETE
      else -> REQ_BODY
    if REQ_BODY:
      if chunked: decode until 0-chunk; produce unchunked body
      else: wait until body.size == Content-Length
      enforce client_max_body_size -> 413 / REQ_ERROR
      -> COMPLETE
  erase consumed prefix from buffer
  return (state == COMPLETE || state == ERROR)
```

### Body expected?

- `GET`/`DELETE` usually no body (if `Content-Length` present, decide: read and ignore, or reject — document; nginx often reads length if present).  
- `POST` with length/chunked: must read.  

### Limits (strongly recommended)

- Max request line length  
- Max header block size  
- Max body size from matched server/location (may need provisional server limit until routed)

If headers never finish → kebris-c timeout should kill the connection; you still must not loop forever busy-spinning.

### Chunked decoding essentials

```text
while true:
  read chunk-size line (hex)
  if size == 0: consume trailing \r\n (and optional trailers) -> done
  read exactly size bytes + \r\n
  append bytes to body
```

Malformed chunk → 400.

### String-only tests (no server required)

Feed partial slices of a request to `parse` in a small `main` or test harness:

1. Full request in one shot.  
2. One character at a time.  
3. Split inside `\r\n\r\n`.  
4. Chunked body in awkward slices.  

This is where your tooling brain wins — automate it in `tests/` even before sockets work.

---

## 6. Response builder

Wire format:

```text
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 123\r\n
Connection: close\r\n
\r\n
<body bytes>
```

### Rules of thumb

- Always set `Content-Length` for in-memory bodies (simplest).  
- Set `Content-Type` from extension (`html`, `css`, `js`, `png`, `jpg`, `txt`, …); default `application/octet-stream`.  
- Redirects: status + `Location` + usually empty/short body.  
- Errors: prefer configured `error_page` file; else built-in HTML in `www/errors/` or generated.  
- `Connection: close` is fine while kebris-c closes after response.

### Status table (minimum)

| Code | When |
|---|---|
| 200 | OK |
| 201 | Created (optional for upload) |
| 204 | No Content (optional for DELETE) |
| 301/302 | Redirect from config |
| 400 | Bad request / malformed |
| 403 | Forbidden (no list permission, etc.) |
| 404 | Not found |
| 405 | Method not allowed |
| 413 | Body too large |
| 500 | CGI/handler failure |
| 501 | Not implemented (temporary during skeleton) |

---

## 7. Router + HttpHandler

### Location match

Longest prefix wins among `location` paths. Example:

- `/` and `/cgi-bin` → request `/cgi-bin/hello.py` matches `/cgi-bin`.

Exact-match locations are optional sugar.

### Filesystem map

1. Reject paths with `..` after normalization (and consider `%2e%2e`).  
2. Strip location prefix from URI path.  
3. Join with `root` (or `alias` if you add it).  
4. `stat`:

| Result | Action |
|---|---|
| Regular file | GET → serve; DELETE → unlink if allowed |
| Directory | index file / autoindex / 403/404 |
| Missing | 404 |

### Method allow-list

If method not in location list → **405**. Optionally send `Allow: GET, POST`.

### Redirect location

If location has `return`/`redirect` → build redirect Response; do not touch disk.

### Autoindex

`opendir` / `readdir` / `closedir` → simple HTML `<a href="...">`. Escape `<`, `&`, `"` in names. Do not expose paths outside the root.

### POST upload

Minimum viable (document choice):

- Raw body saved as a generated filename in `upload_store`, **or**  
- `multipart/form-data` parse for `file` field (harder; only if you need browser form fidelity)

Enforce size → 413. Success → 201/200 + message body.

### DELETE

- Allow only under configured locations/methods.  
- Delete files; be careful with directories (prefer 403 on dirs unless you intentionally support).  

### CGI dispatch

If path ends with `cgi_extension` (or location is CGI-only) and file exists:

1. Resolve script path.  
2. `buildEnv`.  
3. Hand to kebris-c `CgiProcess::start` with **unchunked** body.  
4. When done, `parseCgiOutput`.

---

## 8. CGI contract (your half)

### Environment (build these as `KEY=VALUE` strings)

Minimum useful set:

| Variable | Typical source |
|---|---|
| `GATEWAY_INTERFACE` | `CGI/1.1` |
| `REQUEST_METHOD` | request method |
| `SCRIPT_FILENAME` | absolute script path |
| `SCRIPT_NAME` | URI path to script |
| `PATH_INFO` | extra path after script (often empty) |
| `QUERY_STRING` | without `?` |
| `CONTENT_TYPE` | header or empty |
| `CONTENT_LENGTH` | body size decimal |
| `SERVER_PROTOCOL` | e.g. `HTTP/1.1` |
| `SERVER_NAME` / `SERVER_PORT` | config / listen |
| `REMOTE_ADDR` | `remoteAddr` argument of `prepareCgi` / `Connection::remoteAddr()` |
| `REDIRECT_STATUS` | `200` (helps php-cgi) |

Pass to child as `char *envp[]` (kebris-c builds argv/envp arrays from your strings).

### Working directory

Subject: CGI runs in correct directory for relative paths. kebris-c currently `chdir`s to the **script’s directory** before `execve`. If you need location-root semantics instead, change it jointly — do not assume silently.

### CGI stdout → HTTP

```text
read headers until blank line
if header Status: NNN ... -> use that code
else default 200
remaining bytes -> body
if Content-Length absent, body is until EOF (kebris-c already collected)
synthesize HTTP response with proper status/headers/body
```

### Sample script

`www/cgi-bin/hello.py` is a starting point. Add a POST echo and a script that reads a relative file once cwd is correct.

---

## 9. Phase walkthrough checklists

### Phase 1 — Config

- [ ] `minimal.conf` loads  
- [ ] `default.conf` loads (multi-server)  
- [ ] Bad file → clear error, non-zero exit from `main`  
- [ ] Size suffixes work  
- [ ] Dump/print for visual check  

### Phase 2 — Request/Response + static GET (string-level)

- [ ] Parser tests: full / sliced / bad request  
- [ ] `Response::raw` CRLF correctness  
- [ ] File body + content type  
- [ ] 404 default page  

### Phase 3 — Router features

- [ ] `/` serves index  
- [ ] `/files` autoindex  
- [ ] `/old` redirect  
- [ ] Unknown → 404  
- [ ] Method denial → 405  
- [ ] Traversal attempt → 403/400  

### Phase 4 — Upload / DELETE / 413

- [ ] POST stores under `upload_store`  
- [ ] DELETE removes allowed file  
- [ ] Oversized → 413  
- [ ] Demo form on site (optional but great for eval)  

### Phase 5 — CGI

- [ ] GET CGI works in browser  
- [ ] POST CGI receives body  
- [ ] Query string visible in script  
- [ ] CGI failure → 500  

### Phase 6 — Eval pack

- [ ] Port 8080 and 8081 demos  
- [ ] `tests/test_smoke.py` covers list in `tests/README.md`  
- [ ] nginx comparison notes (short markdown section in tests or README)  
- [ ] README Instructions match reality  

---

## 10. Test plan (use your scraper/tooling skills)

Implement `tests/test_smoke.py` as a real client (`http.client` or raw sockets).

Suggested cases:

1. `GET /` → 200, body contains expected marker  
2. `GET /missing` → 404 + error page HTML  
3. `GET /files/` → 200 listing when autoindex on  
4. `GET /old` → 301/302 + `Location`  
5. `POST /` with body where POST forbidden → 405  
6. `POST /upload` small file → success + file exists on disk  
7. `POST` huge body → 413  
8. `DELETE` allowed vs denied  
9. CGI GET/POST  
10. Port `8081` returns site2 content  
11. Malformed request → 400  
12. Chunked POST (once supported)  

Also keep a **raw socket** slow-client script for kebris-c (1 byte/sec headers).

---

## 11. Failure modes → fixes

| Symptom | Likely yours if… |
|---|---|
| Browser spins forever | parse never COMPLETE (waiting for body) |
| Double-parsed garbage | consumed bytes not erased from `readBuf` |
| CGI empty POST | chunked not decoded; wrong `CONTENT_LENGTH` |
| 404 for real files | root/location join wrong; missing strip of prefix |
| Upload ok locally, eval fails | path not from config; methods open; no size cap |
| Wrong status vs nginx | you guessed; re-check oracle |
| Partner blocked on bind | unstable listen fields / config API churn |

---

## 12. Defense cheat sheet

Practice out loud:

1. “Show how a chunked body becomes CGI stdin.”  
2. “How do you map `/kapouet/...` to disk?”  
3. “Where is `client_max_body_size` enforced?”  
4. “Why don’t you call `recv` in HttpHandler?”  
5. “What status for DELETE on a missing file?”  
6. “Which CGI env vars did you set and why?”  

---

## 13. Definition of done (your half)

- [ ] Config loads both sample files  
- [ ] Incremental parser + response builder correct under slicing  
- [ ] Static GET + index + autoindex + redirects + errors  
- [ ] POST upload + DELETE + 413  
- [ ] One CGI working end-to-end with kebris-c  
- [ ] Multi-port content difference demonstrated  
- [ ] Tests + demo site + README ready for eval  
- [ ] You can explain the poll rule without saying “that’s kebris’s stuff”
- [ ] Every `TODO: This is a workaround` block is gone
- [ ] `default.conf` grammar matches `ServerConfig` / `LocationConfig` fields

Bonus later (yours): cookies + sessions demo. Not now.

---

## 14. Daily loop suggestion

1. Pick one checklist box.  
2. Add a failing test or a `curl -v` expectation first.  
3. Implement in C++98.  
4. Compare weird cases to nginx.  
5. If blocked on “bytes never arrive” → give kebris-c the **hex dump / length** of what you expected vs `readBuf`.  
6. Keep `ServerConfig` fields stable once phase 2 starts.
