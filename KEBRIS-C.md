# kebris-c — full walkthrough (I/O + process plane)

Your Python encrypted chat proves you can think in sockets. It does **not** prove you can pass webserv. The grade-0 traps are almost all in **your** modules: non-blocking I/O, **one** multiplexor, CGI pipes inside that same loop, and never calling `recv`/`send` without readiness.

**Server language: C++98 only.** Python here is CGI payload or external tester. Rules: [`SUBJECT_RULES.md`](SUBJECT_RULES.md).

Partner guide: [`KMARRERO.md`](KMARRERO.md) · Shared phases: [`docs/WORK_SPLIT.md`](docs/WORK_SPLIT.md)

---

## 0. Mindset shift (chat → webserv)

| Your chat (typical) | webserv (subject) |
|---|---|
| Thread per client or blocking `recv` | **One** process, one `poll`/`epoll` |
| App protocol you invented | HTTP/1.x bytes you must not corrupt |
| “Read until newline” in a thread | Incremental buffers; HTTP parse is kmarrero’s, feeding is yours |
| SSL/GUI concerns | Plain TCP + correct readiness |
| Fork rarely | `fork` **only** for CGI |

If you catch yourself writing `while ((n = recv(...)) > 0)` without poll — stop.

---

## 1. Owned files

| Path | Responsibility |
|---|---|
| `Socket.*` | `socket/bind/listen/accept`, `SO_REUSEADDR`, non-blocking |
| `Connection.*` | `readBuf` / `writeBuf`, state, last-activity, send offset |
| `Server.*` | Listen set, connection map, **the** event loop, timeouts, wiring |
| `CgiProcess.*` (process half) | `pipe/fork/dup2/execve/waitpid`, poll callbacks on pipe fds |
| Shared | `main.cpp` bootstrap, `Makefile`, tiny `Utils` — no HTTP logic dump |

kmarrero owns: config structs you **consume**, `Request`/`Response`/`Router`/`HttpHandler`, CGI **env + stdout parse**.

---

## 2. Non-negotiable clarifications

1. **One multiplexor for listen + clients + CGI pipes.** Disk files exempt.  
2. **Monitor read and write.** Enable `POLLOUT` only when you have bytes (or always and no-op — but you must support write readiness).  
3. **No `errno`-driven behaviour after read/write.** Ready → attempt I/O → use return value; on hard failure, close.  
4. **Partial `send` is normal.** Keep `bytesSent` / erase prefix of `writeBuf`.  
5. **`recv == 0`** → peer closed; remove from poll, close fd, free `Connection`.  
6. **Timeouts** so a client dripping headers cannot hold a slot forever.  
7. **No threads.** Not whitelisted; fights the architecture.  
8. **Whitelist only** for syscalls ([`SUBJECT_RULES.md`](SUBJECT_RULES.md)).  
9. **Never `execve` nginx/apache.**  
10. Freeze an interface with kmarrero before phase 3 (see §7).

---

## 3. What to study (ordered)

### Week-zero drills (do before “real” webserv)

1. Tiny C++98 program: one listen socket, `poll`, echo.  
2. Same with **two** listen ports.  
3. Same with forced partial writes (send 1 byte at a time).  
4. `fork` + `pipe`: parent writes “hello”, child `cat`s to stdout — then make pipes non-blocking and drive them with poll.

### Man pages / docs (read for real)

| Read | Focus |
|---|---|
| `man 2 socket` `bind` `listen` `accept` | backlog, `AF_UNSPEC` via getaddrinfo |
| `man 3 getaddrinfo` | `AI_PASSIVE`, freeaddrinfo, IPv4/IPv6 |
| `man 7 ip` / TCP basics | ephemeral ports, `SO_REUSEADDR` why |
| `man 2 poll` **or** `man 7 epoll` | events, revents, hangup/error bits |
| `man 2 recv` `send` | return 0, short counts |
| `man 2 fcntl` | `O_NONBLOCK` only (esp. macOS limits) |
| `man 2 pipe` `dup2` `fork` `execve` `waitpid` | CGI |
| `man 7 signal` / `man 2 sigaction` | `SIGPIPE` (ignore), avoid reaping bugs |
| RFC 3875 (skim) | what CGI expects on stdin/stdout |
| nginx behaviour (observe) | when in doubt later |

**Pick one multiplexor and stick to it.** `poll` is portable and enough; `epoll` is fine on Linux. Do not mix models.

### Concepts you must be able to explain in defense

- Why readiness is required before `recv`/`send` on sockets/pipes  
- Difference between “socket readable” and “HTTP request complete”  
- Why CGI pipes belong in the **same** poll set  
- How you avoid hanging forever  
- Why `fork` for every HTTP request would be wrong here  

---

## 4. Recommended internal design

```text
Server
  vector of listen fds (+ which ServerConfig they belong to)
  map<fd, Connection*>
  map<fd, CgiProcess*>   // or Connection holds optional CgiProcess*
  pollfd[] rebuilt each iteration (or epoll ctl updates)

Connection
  fd
  state: READING | PROCESSING | WRITING | CLOSING
  readBuf, writeBuf, writeOffset
  Request req                      // kmarrero type
  lastActivity
  optional: cgi pointer / serverConfig index
```

### State machine (keep it boring)

```text
READING
  POLLIN -> recv append readBuf
  ask Request::parse(readBuf)          // kmarrero
  if error -> build error Response -> WRITING
  if complete ->
      if CGI needed -> start CgiProcess -> PROCESSING
      else Response = HttpHandler -> WRITING

PROCESSING (CGI)
  poll cgi pipes
  when CGI DONE -> Response from parseCgiOutput -> WRITING

WRITING
  POLLOUT -> send from writeBuf+offset
  if done -> close (simple) OR reset for keepalive (optional later)

CLOSING
  remove from maps, close fd
```

Start with **close after one response**. Keepalive is optional polish; correctness first.

### Poll rebuild pattern (poll-based pseudocode)

```text
each loop:
  pfds.clear()
  for listenFd: pfds.push({listenFd, POLLIN})
  for each conn:
      events = 0
      if state == READING or needs more body: events |= POLLIN
      if writeBuf not fully sent: events |= POLLOUT
      // also consider POLLIN during WRITING if you support pipelining later
  for each cgi pipe fd: add POLLIN and/or POLLOUT as needed
  poll(pfds, timeout_ms)   // timeout_ms also drives idle sweeps
  handle listen accepts
  handle client read/write
  handle cgi fds
  sweep timeouts
```

---

## 5. Phase walkthrough

### Phase 1 — Echo (unblock yourself)

**Goal:** Prove the loop before HTTP exists.

- [ ] `Socket::listenOn("127.0.0.1", 8080)` works; `SO_REUSEADDR` set  
- [ ] Non-blocking listen + clients  
- [ ] `poll` loop; accept on `POLLIN`  
- [ ] Echo `readBuf` → `writeBuf`  
- [ ] Partial send handled  
- [ ] Disconnect cleanup (no fd leaks — check `/proc/self/fd` or `ls -l /proc/$PID/fd`)  

Temporary: hardcode port if config not ready. Delete hardcoded path once `Config::load` works.

**Self-test**

```bash
printf 'hello' | nc 127.0.0.1 8080
# second terminal: another nc simultaneously — both must work
```

### Phase 2 — Multi-port + Connection + timeouts

- [ ] Create one listen socket per `ServerConfig`  
- [ ] Map `listenFd → config index`  
- [ ] `Connection::touch` on I/O; close if idle > N seconds  
- [ ] `poll` timeout (e.g. 500–1000 ms) so sweeps run even without traffic  

**Self-test:** listen `8080` and `8081`; echo on both.

### Phase 3 — Wire kmarrero HTTP (pair session)

Agree on API in §7. Then:

- [ ] On read: append bytes; call `Request::parse`  
- [ ] Consume parsed bytes from `readBuf` (kmarrero should erase/consume — agree who does)  
- [ ] On complete: `Router` + `HttpHandler` → `writeBuf = response.raw()`  
- [ ] Switch to write interest; send until done; close  

**First vertical slice:** `GET /` → `www/index.html` in a real browser.

Debug tip: log (temporarily) sizes of `readBuf`/`writeBuf` and parse state — remove noisy logs before eval if required by peer norms.

### Phase 4 — Bodies & backpressure

- [ ] Large POST does not block other clients  
- [ ] If body exceeds max size, cooperate with kmarrero’s 413 (you may stop reading / close after error response)  
- [ ] Bound memory: do not let one client grow `readBuf` without limit  

**Self-test:** upload a few MB while curling `/` in a loop.

### Phase 5 — CGI process plane

Subject constraints baked in:

- `fork` only for CGI  
- Pipes non-blocking + **same** poll  
- Write full unchunked body to CGI stdin, then **close write end** (EOF)  
- Read stdout until EOF; `waitpid(WNOHANG)` in loop  

Checklist:

- [ ] `pipe` for stdin, `pipe` for stdout (stderr: inherit, redirect to `/dev/null`, or separate pipe — decide and document)  
- [ ] Child: `dup2`, close extras, `chdir` to correct dir, `execve(cgi_pass, argv, envp)`  
- [ ] Parent: close child ends; register parent ends in poll  
- [ ] `onPipeWritable` / `onPipeReadable` / `tryReap`  
- [ ] On failure (`execve` fail): return 500 path to kmarrero  

**Self-test:** `curl -v 'http://127.0.0.1:8080/cgi-bin/hello.py?x=1'` and POST body variant.

Hang checklist if CGI stuck:

1. Did you close CGI stdin after body?  
2. Are pipe fds in poll?  
3. Is `waitpid` blocking the loop? (must be `WNOHANG`)  
4. Is the script executable / interpreter path correct in config?  

### Phase 6 — Stress & resilience

- [ ] 200 parallel `curl`s survive  
- [ ] Client disconnect mid-headers mid-body  
- [ ] Slowloris-ish drip (partner can script) → timeout, server lives  
- [ ] CGI crash / early exit → 500, server lives  
- [ ] No crash on huge headers if you impose a max header size (good practice; coordinate with parser)  

```bash
for i in $(seq 1 200); do curl -s -o /dev/null http://127.0.0.1:8080/ & done; wait
```

---

## 6. Socket / accept details worth getting right

- Use `getaddrinfo` for `host:port` (supports `0.0.0.0` / `127.0.0.1`).  
- `setsockopt(SO_REUSEADDR, 1)` before `bind`.  
- `listen(fd, backlog)` — backlog 128 is a common starting point.  
- After `accept`, **immediately** set client non-blocking.  
- Handle accept errors that mean “try again later” without killing the server.  
- Track `POLLHUP` / `POLLERR` / `POLLNVAL` → close connection.  

### SIGPIPE

Sending to a closed peer can raise `SIGPIPE` and kill the process. Typical approach: ignore `SIGPIPE` at startup (`signal(SIGPIPE, SIG_IGN)` is on the whitelist). Still handle short/failed sends by closing the connection.

---

## 7. Integration contract with kmarrero (freeze early)

Copy this into a shared note and tick when both agree:

```text
[ ] ServerConfig / LocationConfig fields stable
[ ] Request::parse(std::string &buf) incremental; returns complete/error
[ ] Who erases consumed bytes from readBuf? (prefer Request::parse)
[ ] How HttpHandler signals "needs CGI" (flag / separate method / empty cgi path)
[ ] CgiProcess::buildEnv(...) owned by kmarrero
[ ] parseCgiOutput(string) -> Response owned by kmarrero
[ ] Max header size / max body size sources (config)
[ ] Error Response always available even if handler throws/returns false
```

### Call flow you implement

```text
POLLIN(client):
  n = recv(...)
  if n == 0 -> close
  if n > 0 -> readBuf.append; touch()
  if req.parse(readBuf) complete:
      if route says CGI:
          env = CgiProcess::buildEnv(...)
          cgi.start(loc, script, env, req.body())
          state = PROCESSING
      else:
          writeBuf = HttpHandler.handle(...).raw()
          state = WRITING

POLLOUT(client):
  n = send(fd, writeBuf.data()+off, remaining)
  advance off; if done -> close/reset

CGI pipes:
  writable -> cgi.onPipeWritable()
  readable -> cgi.onPipeReadable()
  when DONE -> writeBuf = parseCgiOutput(cgi.output()).raw(); state = WRITING
```

You **do not** parse HTTP headers yourself beyond maybe detecting completeness if you temporarily stub — final parser is kmarrero’s.

---

## 8. Failure modes → fixes

| Symptom | Check |
|---|---|
| Server freezes with one client | Blocking call; missing non-blocking; `waitpid` without `WNOHANG` |
| Works once then dead | Failed to reset poll flags / leaked state / listen not polled |
| Browser spins | Never switching to WRITING; incomplete response; forgot final `send` |
| curl OK, browser bad | Often HTTP layer — but also check you handle multiple requests/connections |
| `Address already in use` | Old process alive; missing `SO_REUSEADDR` |
| CGI hang | stdin not closed; pipes not polled |
| Eval “grade 0 I/O” speech | `recv`/`send` without readiness; second hidden loop |
| Growing memory | unbounded buffers; connections not closed |

---

## 9. Defense cheat sheet (your mouth, not notes)

Practice answering out loud:

1. “Show me where poll waits for write.”  
2. “What happens if `send` writes half the response?”  
3. “Why are CGI pipes in the same poll?”  
4. “What if the client never finishes headers?”  
5. “Which functions are allowed for sockets?”  
6. “Why not thread per client like your chat app?”  

If you cannot answer without reading code, you are not done.

---

## 10. Definition of done (your half)

- [ ] Single poll/epoll loop: listen + clients + CGI pipes  
- [ ] Multi-port from config  
- [ ] Non-blocking everywhere that can block  
- [ ] Partial I/O + disconnects + timeouts  
- [ ] CGI child works without blocking the loop  
- [ ] Stress does not kill `webserv`  
- [ ] No forbidden APIs  
- [ ] You can explain kmarrero’s request/response path at a high level  

Bonus later: multiple CGI executors = data-driven `cgi_pass` (you already should avoid hardcoding `/usr/bin/python3` in C++).

---

## 11. Daily loop suggestion

1. Implement one checklist item.  
2. Run a concrete `nc`/`curl` repro.  
3. If blocked on HTTP semantics → ping kmarrero with a **failing raw request string**, not “HTTP is broken.”  
4. Commit small.  
5. Re-read §2 before adding any new syscall.
