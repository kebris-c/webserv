# Work split, interfaces, and build order

Mandatory only. Bonus is out of scope until mandatory is boringly stable.

**Server language: C++98.** Python only for CGI scripts and external tests.
Compliance map: [`../SUBJECT_RULES.md`](../SUBJECT_RULES.md).
Deep guides: [`../KEBRIS-C.md`](../KEBRIS-C.md) · [`../KMARRERO.md`](../KMARRERO.md).
Integration ledger (workarounds / seams): [`README.md`](README.md).

---

## Current integration checkpoint

- **Multiplexor is frozen to `poll`.** Do not add `epoll`, `select`, or a second wait loop.
- kebris-c transport is **closed for mandatory I/O**: non-blocking listeners/clients,
  multi-port mapping, partial writes, bounded admission/buffers, disconnect cleanup,
  idle timeouts + phase deadlines, CGI process pipes/reaping, signal stop, accept
  backoff, and peer IPv4 via `Connection::remoteAddr()` passed into `prepareCgi`.
- `src/main.cpp` still falls back to temporary TCP echo on `127.0.0.1:8080` and
  `:8081` because `Config::load` is a kmarrero stub. Echo proves transport only; it
  is **not HTTP**.
- `HttpHandler::prepareCgi` and `HttpHandler::parseCgiOutput` remain explicit
  integration stubs for kmarrero to replace (see [`README.md`](README.md) §3).

**Removal gate:** delete every highlighted workaround only after config loading,
incremental request parsing, normal response generation, and CGI response parsing pass
string tests and a browser `GET /` works.

---

## Uncomfortable rule

Equal effort ≠ equal file count.

- **kebris-c** → fewer files, most grade-0 I/O risk  
- **kmarrero** → more surface (HTTP/config/demo/tests)  

Both must explain both planes in defense. Ownership is for shipping speed, not for secrecy.

---

## Ownership map

| Owner | Owns | Does not own |
|---|---|---|
| **kebris-c** | sockets, `poll` loop, connections, CGI **process/pipes**, stress of the loop | config grammar, HTTP header semantics, HTML autoindex content |
| **kmarrero** | config parser, request/response, router, handlers, CGI **env/output**, www, tests | calling `recv`/`send`, inventing a second I/O wait model |
| **both** | `main` glue, Makefile, Utils, README truthfulness, integration sessions, defense prep | — |

---

## Frozen interface (agree in writing before phase 3)

```text
Config::load(path) -> vector<ServerConfig>

Server (kebris-c)
  binds every ServerConfig listen host:port
  poll(listen + clients + cgi pipes)
  Connection { readBuf, writeBuf, Request, state, timeout }

Request::parse(readBuf) -> complete | error     # kmarrero, incremental, consumes bytes
Router::match(server, request) -> RouteMatch    # kmarrero
HttpHandler::handle(...) -> Response            # or "needs CGI"
HttpHandler::prepareCgi(...) -> bool + path/env # + remoteAddr string from Connection
CgiProcess::buildEnv(...)                       # kmarrero (must set REMOTE_ADDR=)
CgiProcess::start / pipe poll callbacks         # kebris-c
HttpHandler::parseCgiOutput(stdout) -> Response # kmarrero
Response::raw() -> bytes into writeBuf          # kmarrero build, kebris-c send
```

Hard rule: socket/pipe `read`/`recv`/`write`/`send` only after poll readiness. Disk files exempt.

---

## Phased order (do not skip vertically)

| Phase | kebris-c | kmarrero | Merge gate |
|---|---|---|---|
| **1** | poll echo, 1 port | config → structs for `minimal.conf` | config can describe a port |
| **2** | multi-port + Connection + timeouts | Request/Response string tests + static GET builder | API of Request/Response stable |
| **3** | wire parse → handler → send | Router: methods, redirect, autoindex, errors | **browser GET /** works |
| **4** | body/backpressure | POST upload, DELETE, 413 | upload+delete demos |
| **5** | CGI pipes in **same** poll | CGI env + stdout→HTTP | browser CGI works |
| **6** | stress / disconnect / hang hunt | smoke tests, nginx compare, README, demo polish | eval-ready |

If phase N is red, do not start N+2 features on top.

---

## Pair sessions (schedule these)

1. **Kickoff (2–3h):** read `SUBJECT_RULES.md` together; freeze `ServerConfig`/`LocationConfig` fields. **`poll` is already chosen.**
2. **First GET (phase 3):** sit together until browser shows `www/index.html`.  
3. **CGI (phase 5):** env vars + pipe EOF checklist on one machine.  
4. **Defense rehearsal:** each explains the other’s module with the PDF open.

---

## Definition of done (team)

- [ ] `./webserv configs/default.conf` serves `www/` in a real browser  
- [ ] GET / POST / DELETE per location rules  
- [ ] Upload to configured store  
- [ ] ≥1 CGI works  
- [ ] Multi-port works (`8080` / `8081` samples)  
- [ ] Default error pages  
- [ ] Stress: process stays up  
- [ ] No blocking socket/pipe I/O outside poll  
- [ ] Whitelist-only syscalls; C++98; no Boost/threads  
- [ ] README still matches subject Ch. V  

---

## When blocked

| Blocked on | Do this |
|---|---|
| “Bytes don’t arrive” | kebris-c: log recv lengths; kmarrero: provide expected request bytes |
| “Parse never completes” | kmarrero: reproduce with string-only slices; check Content-Length |
| “CGI hangs” | joint: stdin closed? pipes in poll? `waitpid WNOHANG`? |
| “Status disagrees with nginx” | kmarrero owns alignment; capture both `curl -v` outputs |
| “Which syscall is allowed?” | `SUBJECT_RULES.md` — if absent, don’t use it |
