# kebris-c — walkthrough (I/O + process plane)

You already built a multi-user socket chat in Python. That helps with *mental models* — it does **not** make webserv easy. The grade-0 traps live in **your** files: non-blocking I/O, a **single** poll/epoll loop, and never calling `recv`/`send` on sockets/pipes without readiness.

Partner: **kmarrero** owns HTTP/config/content. You must still be able to explain their half in defense.

Related: [`docs/WORK_SPLIT.md`](docs/WORK_SPLIT.md) · partner guide: [`KMARRERO.md`](KMARRERO.md)

---

## Your owned files

| Path | Your job |
|---|---|
| `include/Socket.hpp`, `src/Socket.cpp` | listen/accept, non-blocking fds |
| `include/Connection.hpp`, `src/Connection.cpp` | per-client buffers, state, timeout |
| `include/Server.hpp`, `src/Server.cpp` | **the** event loop; multi-port; wire handlers |
| `include/CgiProcess.hpp`, `src/CgiProcess.cpp` | fork/pipes/`execve` + poll integration (**env/output parse = kmarrero**) |
| Shared: `src/main.cpp`, `Makefile`, `Utils.*` | bootstrap only — do not dump HTTP logic here |

---

## Clarifications (read before coding)

1. **One multiplexing call for all network/pipe I/O**  
   Listen sockets, client sockets, CGI pipes: same `poll`/`epoll`. Disk `open`/`read`/`write` for static files do **not** need poll.

2. **`errno` after read/write**  
   Subject: do **not** use `errno` to adjust server behaviour after a read/write. Design around readiness + return values (`0` = peer close, `-1` with would-block handling decided *before* you rely on errno games). Prefer checking poll flags and treating short reads/writes as normal.

3. **Non-blocking is mandatory**  
   `accept` / `recv` / `send` / CGI pipe I/O must not stall the whole server. Partial sends are normal — keep a write buffer + offset.

4. **Fork only for CGI**  
   No worker-process-per-request architecture.

5. **Requests must not hang forever**  
   Idle/header/body timeouts; close stale connections in the loop.

6. **Your chat project ≠ this**  
   Thread-per-client or blocking `recv` loops will fail the subject. Multiplex in **one** thread/process loop.

7. **Interface with kmarrero**  
   - You append bytes into `Connection::readBuf()`.  
   - They parse with `Request::parse(buffer)` (incremental).  
   - They produce `Response::raw()` (or CGI job).  
   - You only `send` from `writeBuf` when `POLLOUT`.

Agree buffer ownership and “request complete” signalling in writing before phase 3.

---

## Knowledge to learn / investigate

### Must know cold

| Topic | Where | Why |
|---|---|---|
| `socket` `bind` `listen` `accept` `setsockopt(SO_REUSEADDR)` | `man 2 …` | multi-port listeners |
| `getaddrinfo` / `freeaddrinfo` | `man 3 getaddrinfo` | host:port setup |
| `fcntl` + `O_NONBLOCK` | `man 2 fcntl` | subject; macOS flag limits if you ever build there |
| `poll` **or** `epoll` | `man 2 poll` / `man 7 epoll` | pick one; monitor **IN and OUT** |
| Partial `recv`/`send` | experiments with `curl` / slow `nc` | writeBuf state machine |
| `pipe` `fork` `dup2` `execve` `waitpid` | CGI | child lifecycle |
| Signal policy for zombies / `SIGPIPE` | `signal` / `sigaction` | ignore `SIGPIPE` or handle `EPIPE` carefully without violating errno rule spirit |

### Expand investigation

- How nginx handles keepalive vs close (you can start with close-after-response).
- Difference between level-triggered poll and edge-triggered epoll (if you choose epoll, know the traps).
- What happens when client stops reading while you still have a large `writeBuf` (backpressure: stop reading new requests or stop filling buffer unboundedly).
- CGI stdin must see **EOF** after body; unchunking is kmarrero’s job — you write already-unchunked bytes.
- Stress: thousands of connect/close, slowloris-style header drip, disconnect mid-body.

### Useful experiments (do these)

```bash
# Compare with a real server
nginx  # or docker nginx — curl -v and note status/headers

# Raw client
printf 'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n' | nc 127.0.0.1 8080

# Partial write pressure later
# Use Python to send 1 byte/sec headers — your timeout + parse must survive
```

Build a **throwaway echo server** first (phase 1) before wiring HTTP.

---

## Walkthrough by phase

### Phase 1 — Echo server (solo unblock)

**Done when:** `./webserv` (hardcode one port temporarily if config not ready) accepts many clients, `poll` loop runs, bytes echo back, disconnects cleaned up.

Checklist:

- [ ] `Socket::listenOn` + `setNonBlocking`
- [ ] `Server::run` poll loop over listen + clients
- [ ] `POLLIN` → `recv` into buffer → queue same bytes to `writeBuf`
- [ ] `POLLOUT` only when `writeBuf` non-empty
- [ ] Peer close (`recv == 0`) → remove from poll map, close fd

**Do not wait** for a perfect config parser — mock one listen fd if needed; integrate `Config` as soon as kmarrero can load `minimal.conf`.

### Phase 2 — Multi-port + real Connection object

**Done when:** two listen ports from config both accept; each client has `Connection` with timeout stamp.

Checklist:

- [ ] Map `listenFd → ServerConfig` (or index)
- [ ] `Connection::touch` / `timedOut`
- [ ] No blocking `accept` loop without returning to poll

### Phase 3 — Wire HTTP parse/respond (pair with kmarrero)

**Done when:** browser `GET /` returns kmarrero’s static response through your send path.

Checklist:

- [ ] On read: `Request::parse(readBuf)` until complete/error
- [ ] Consume parsed bytes from `readBuf` correctly (do not reparse forever)
- [ ] Call into Router/HttpHandler **or** a thin callback kmarrero provides
- [ ] `writeBuf = response.raw()`; enable `POLLOUT`
- [ ] After full send: close or reset for keepalive (keepalive optional; closing is OK at first)

### Phase 4 — Bodies / backpressure

**Done when:** large POST bodies stream into `readBuf` without freezing other clients; oversized bodies can be rejected (kmarrero decides 413; you must keep loop alive).

Checklist:

- [ ] Still only one poll loop
- [ ] Cap buffer growth or stop `POLLIN` when limits hit
- [ ] Slow clients cannot stall others

### Phase 5 — CGI process plane

**Done when:** `www/cgi-bin/hello.py` runs via fork/exec; pipes are in **the same** poll set; response returns to browser.

Your responsibilities:

- [ ] `pipe` + `fork` + `dup2` + `execve(cgi_pass, …)`
- [ ] Parent ends non-blocking; register stdin/stdout pipe fds in poll
- [ ] `onPipeWritable` / `onPipeReadable` / `tryReap`
- [ ] Close CGI stdin after body fully written (EOF for CGI)

kmarrero responsibilities (block on them, do not reimplement):

- `CgiProcess::buildEnv`
- Parsing CGI stdout (headers + body) into `Response`

### Phase 6 — Stress & hang hunting

**Done when:** server stays up under parallel curls; hung clients get timed out; no crash on abrupt TCP close.

Suggested personal tests:

```bash
# Parallel GETs
for i in $(seq 1 200); do curl -s -o /dev/null http://127.0.0.1:8080/ & done; wait

# Disconnect mid-request
( printf 'GET / HTTP/1.1\r\nHost: x\r\n'; sleep 0.1; ) | nc 127.0.0.1 8080 &
sleep 0.2; killall nc 2>/dev/null || true
```

Partner’s `tests/` should eventually automate this; you still own loop correctness.

---

## Integration contract (copy into a shared note if needed)

```text
Server (you)
  on POLLIN(client):
    recv -> conn.readBuf
    req.parse(conn.readBuf)          # kmarrero
    if req complete:
      if needs CGI:
        env = CgiProcess::buildEnv() # kmarrero
        cgi.start(...)               # you
      else:
        resp = HttpHandler.handle()  # kmarrero
        conn.writeBuf = resp.raw()
  on POLLOUT(client):
    send from writeBuf
  on POLLIN/POLLOUT(cgi pipes):
    cgi.onPipe*
    when cgi DONE:
      resp = parseCgiOutput()        # kmarrero
      conn.writeBuf = resp.raw()
```

---

## Common failure modes (yours)

| Symptom | Likely cause |
|---|---|
| Whole server freezes on one client | Blocking read/write or forgot non-blocking |
| Works with curl, dies with browser | Incomplete headers/body wait; or only one request then stuck poll flags |
| CGI hangs | Never closed CGI stdin; pipes not in poll; `waitpid` blocking |
| Port already in use after crash | Missing `SO_REUSEADDR` or zombie process still bound |
| Grade 0 speech from evaluator | `recv` without POLLIN, or multiple ad-hoc blocking loops |

---

## Definition of done (your half)

- [ ] Single poll/epoll loop drives listen + clients + CGI pipes
- [ ] Multi-port listen works with kmarrero’s config
- [ ] Partial reads/writes handled; disconnects safe
- [ ] Timeouts prevent infinite hangs
- [ ] CGI child runs without blocking the loop
- [ ] Stress does not kill the process
- [ ] You can explain **why** every `recv`/`send` is legal under the subject

Bonus later (not now): multiple CGI executors reuse this launcher — keep `cgi_pass` data-driven, not hardcoded to Python only.

---

## Suggested study order (short)

1. Rewrite a tiny C/C++ poll echo server from scratch (1 listen port).  
2. Add non-blocking + multi-client.  
3. Read RFC 9112 only enough to know where a request ends (kmarrero parses; you must feed bytes correctly).  
4. RFC 3875 §4 (CGI env) — skim; implement process side.  
5. Break your server on purpose (slow client, huge body, kill nc) and fix.
