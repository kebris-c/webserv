# kmarrero — walkthrough (HTTP + config + content plane)

Scrapers and web tooling give you an edge on **headers, status codes, forms, and configs**. They do not replace reading how HTTP messages are framed on the wire. If the request parser is wrong, kebris-c’s poll loop will look “buggy” when the bug is yours.

Partner: **kebris-c** owns sockets/poll/CGI *process*. You must still explain that half in defense — especially “why no `recv` without poll”.

Related: [`docs/WORK_SPLIT.md`](docs/WORK_SPLIT.md) · partner guide: [`KEBRIS-C.md`](KEBRIS-C.md)

---

## Your owned files

| Path | Your job |
|---|---|
| `include/Config.hpp`, `src/Config.cpp` | parse nginx-inspired configs → structs |
| `configs/default.conf`, `configs/minimal.conf` | evaluation configs that prove features |
| `include/Request.hpp`, `src/Request.cpp` | incremental HTTP request parser |
| `include/Response.hpp`, `src/Response.cpp` | status/headers/body → wire bytes |
| `include/Router.hpp`, `src/Router.cpp` | location match + URL→filesystem mapping |
| `include/HttpHandler.hpp`, `src/HttpHandler.cpp` | GET/POST/DELETE/redirect/autoindex/upload |
| `CgiProcess::buildEnv` + CGI stdout→HTTP | CGI *contract* (process = kebris-c) |
| `www/**`, `tests/**` | demo site + automated checks |
| Shared: README polish, `Utils.*` helpers | docs/tests accuracy |

---

## Clarifications (read before coding)

1. **You do not call `recv`/`send`**  
   Work on `std::string` buffers. kebris-c owns socket I/O. If you “just read the socket in the handler”, you can zero the project.

2. **Parser must be incremental**  
   Bytes arrive in pieces. `Request::parse(buffer)` should resume across calls and consume data from the buffer when headers/body complete.

3. **Chunked bodies**  
   Subject: unchunk before CGI. CGI expects a plain body and EOF. Implement chunk decode in the request parser (or a dedicated helper you own).

4. **Virtual hosts are optional**  
   Subject says virtual host is out of scope. You may ignore `Host`-based vhosts at first; still parse `server_name` if you want later. Multi-**port** is mandatory (multiple `listen` entries).

5. **Accurate status codes**  
   Not optional flavour — evaluators check them. Build a small table (200, 201, 204, 301, 302, 400, 403, 404, 405, 413, 500, 501…) and default HTML error pages (`www/errors/`).

6. **Config is part of the product**  
   Evaluation uses *your* config files. `configs/default.conf` must demonstrate: multi-port, methods, redirect, autoindex, upload path, CGI extension, body size, error pages.

7. **nginx is the behaviour oracle**  
   When unsure (trailing slash, redirect, autoindex listing, method denial), compare with nginx. Do not invent a second HTTP dialect.

8. **C++98**  
   No C++11 move semantics, no ranged-for, no `std::to_string` (use `stringstream`), no `<unordered_map>` unless you confirm your toolchain/subject comfort — stick to `std::map` / `std::vector`.

---

## Knowledge to learn / investigate

### Must know cold

| Topic | Resource | Why |
|---|---|---|
| Request line + headers + body framing | [RFC 9112](https://www.rfc-editor.org/rfc/rfc9112.html) | parser correctness |
| Method semantics / status codes | [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110.html) | GET/POST/DELETE + errors |
| nginx `server` / `location` / `root` / `index` / `error_page` | [nginx docs](https://nginx.org/en/docs/beginners_guide.html) | config model |
| CGI environment & response | [RFC 3875](https://datatracker.ietf.org/doc/html/rfc3875) | `buildEnv` + stdout parse |
| MIME types basics | extension → `Content-Type` | static site |
| HTML form upload / multipart (as needed) | MDN form encoding | POST upload |
| Directory listing safety | HTML-escape names; block `..` | autoindex + path map |

### Expand investigation

- **Longest-prefix location match** (no regex required). Example from subject: `/kapouet` rooted at `/tmp/www`.
- **Path traversal**: `/%2e%2e/`, `/../../etc/passwd` — reject before `open`.
- **`client_max_body_size`**: enforce → **413**; agree with kebris-c when to stop reading.
- **Directory requests**: use `index`, else autoindex on, else 403/404 — decide consistently with nginx.
- **DELETE**: file vs directory policy; do not casually `rm -rf` trees unless you intentionally support it.
- **CGI output**: headers until blank line; optional `Status:` header; if no `Content-Length`, EOF ends body (kebris-c must deliver EOF).
- **Browser quirks**: Chrome may send multiple requests (favicon). Server must keep serving.

### Useful experiments

```bash
# See a real response
curl -v http://127.0.0.1:8080/

# Force methods
curl -v -X DELETE http://127.0.0.1:8080/files/readme.txt
curl -v -X POST --data-binary @file.bin http://127.0.0.1:8080/upload

# Chunked POST (once supported)
curl -v -H 'Transfer-Encoding: chunked' -d @file.bin http://127.0.0.1:8080/upload

# Compare with nginx on the same paths/status lines
```

Write Python tests early (`tests/test_smoke.py`) — that matches your tooling strength and catches regressions when kebris-c changes the loop.

---

## Walkthrough by phase

### Phase 1 — Config parser

**Done when:** `Config::load("configs/minimal.conf")` fills `ServerConfig` + `LocationConfig`; `default.conf` loads without crashing.

Checklist:

- [ ] Ignore `#` comments and whitespace
- [ ] Parse `server { ... }` and nested `location <path> { ... }`
- [ ] Directives: `listen`, `root`, `index`, `error_page`, `client_max_body_size`, `allowed_methods`, `return`/`redirect`, `autoindex`, `upload_store`, `cgi_extension`, `cgi_pass`
- [ ] Sizes like `10M` → bytes
- [ ] Clear error messages on malformed config (stderr is fine)

Tip: tokenize on spaces/braces first; do not overbuild a full nginx clone.

Deliverable for kebris-c: they can iterate `config.servers()` and bind each `host:port`.

### Phase 2 — Request + Response + static GET

**Done when:** given a full request string, you produce a correct `HTTP/1.1` response for a file under `www/`.

Checklist:

- [ ] Parse method, target, version, headers
- [ ] Split query string from path
- [ ] `Content-Length` body accumulation
- [ ] `Response::raw()` correct CRLFs
- [ ] `Content-Type` for html/css/txt/png/… (start small)
- [ ] Default 404 page from config or `www/errors/404.html`

Unit-test the parser with raw strings **before** integration (no sockets needed).

### Phase 3 — Router + methods + redirect + autoindex + errors

**Done when:** `configs/default.conf` behaviours match expectations for `/`, `/files`, `/old`, unknown paths.

Checklist:

- [ ] Longest prefix location match
- [ ] Map URI → filesystem path (subject `/kapouet` example)
- [ ] Reject `..` / escaped traversal
- [ ] Method allow-list → **405** + `Allow` header (nice to have / nginx-like)
- [ ] Redirect location → **301/302** + `Location`
- [ ] Autoindex HTML via `opendir`/`readdir`/`closedir`
- [ ] Directory + index file behaviour

### Phase 4 — POST upload + DELETE + body limits

**Done when:** upload lands in `www/uploads` (or configured store); DELETE removes allowed resources; oversized body → 413.

Checklist:

- [ ] Enforce max body size during parse
- [ ] POST writes file safely (unique name or from headers — document choice)
- [ ] DELETE returns sensible codes (200/204/403/404)
- [ ] Update demo page with a minimal upload form

Coordinate with kebris-c: they must keep reading sockets while bodies arrive; you define when the request is “complete”.

### Phase 5 — CGI contract

**Done when:** browser hits `/cgi-bin/hello.py` and sees script output; POST body reaches the script.

Your checklist:

- [ ] Detect CGI by extension + `cgi_pass` from location
- [ ] `CgiProcess::buildEnv` — `REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_LENGTH`, `CONTENT_TYPE`, `SCRIPT_FILENAME` / `SCRIPT_NAME`, `PATH_INFO`, `SERVER_PROTOCOL`, `SERVER_PORT`, `GATEWAY_INTERFACE`, etc.
- [ ] Working directory expectation documented for kebris-c (`chdir` in child)
- [ ] Parse CGI stdout → HTTP response (Status/Content-Type/body)
- [ ] Unchunked body only on CGI stdin

kebris-c checklist (you depend on it): non-blocking pipes inside **their** poll loop, EOF on stdin, `waitpid` without blocking the server.

### Phase 6 — Demo, nginx comparison, tests, README

**Done when:** evaluation can be driven from README + `configs/default.conf` + browser; `tests/test_smoke.py` covers the checklist in `tests/README.md`.

Checklist:

- [ ] Static site looks intentional on ports **8080** and **8081**
- [ ] Error pages exist and are referenced
- [ ] Python smoke tests for GET/404/POST/DELETE/redirect/CGI/multi-port
- [ ] README Instructions still accurate
- [ ] Note how AI was used stays honest

---

## Integration contract (what you owe kebris-c)

```text
Config::load(path) -> vector<ServerConfig>

Request::parse(std::string &readBuf) -> bool (complete or error)
  - strips consumed bytes from readBuf
  - sets errorCode on bad requests

Router::match(server, request) -> RouteMatch (location + fsPath)

HttpHandler::handle(request, route) -> Response
  OR signals "needs CGI" with script path + env

CgiProcess::buildEnv(...) -> vector<string> KEY=VALUE
parseCgiOutput(cgi.stdout bytes) -> Response
```

Keep APIs C++98-friendly and free of socket calls.

---

## Common failure modes (yours)

| Symptom | Likely cause |
|---|---|
| Browser spins forever | Parser never reaches COMPLETE (waiting for body that will not come) |
| CGI gets empty POST | Chunked not unchunked; or `CONTENT_LENGTH` wrong |
| 404 for valid files | `root` + location prefix join wrong |
| Works in curl, not browser | Missing `Host` handling is usually OK; more often bad `Content-Length` or incomplete response CRLF |
| Upload “works” but eval fails | Path not from config; methods not restricted; no size limit |
| Partner blocked | Config API unstable — freeze `ServerConfig` fields early |

---

## Definition of done (your half)

- [ ] Config loads `minimal.conf` and `default.conf`
- [ ] Incremental request parser + solid response builder
- [ ] GET static + autoindex + index + redirects + error pages
- [ ] POST upload + DELETE + 413 on huge body
- [ ] CGI env + CGI response parsing for at least one script
- [ ] Demo site + tests + nginx comparison notes
- [ ] You can explain status codes and location matching without reading notes

Bonus later (not now): cookies/sessions are mostly **your** feature (Set-Cookie, session map, tiny login demo). Do not start until mandatory is boringly stable.

---

## Suggested study order (short)

1. Manually write 5 HTTP requests in a text file; parse them on paper.  
2. Implement config → structs; print dump.  
3. Implement `Request`/`Response` with string-only unit tests.  
4. Router + static GET; plug into kebris-c when echo server exists.  
5. Upload/DELETE/CGI contract.  
6. Automate with Python; break cases on purpose (bad headers, huge body, `..` paths).
