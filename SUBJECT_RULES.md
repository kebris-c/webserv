# Subject rules — allowed vs forbidden (webserv v24.0)

Re-read the official PDF when in doubt. This file is the team compliance map so nobody confuses **CGI/test Python** with the **C++98 server**, and so grade-0 rules stay visible while coding.

Cross-links: [`KEBRIS-C.md`](KEBRIS-C.md) · [`KMARRERO.md`](KMARRERO.md) · [`docs/WORK_SPLIT.md`](docs/WORK_SPLIT.md)

---

## 1. What you are building

| Item | Rule |
|---|---|
| Program name | `webserv` |
| Language | **C++98 only** for the server |
| Run | `./webserv [configuration file]` (or a default config path) |
| Build | `c++` + `-Wall -Wextra -Werror`; must still compile with `-std=c++98` |
| Libraries | **No** external libraries, **no Boost** |
| Prefer C++ headers | e.g. `<cstring>` over `<string.h>` when both exist |
| Core submit set | `Makefile`, `*.{h,hpp}`, `*.cpp`, `*.tpp`, `*.ipp`, configuration files |
| Also needed in practice | demo `www/`, error pages, CGI scripts, README (subject Ch. V) |

The binary must not crash or die unexpectedly (including under memory pressure) or the project can be graded **non-functional / 0**.

---

## 2. Language boundary (Python confusion)

### Illegal

- Implementing the HTTP server in Python
- Linking Python into `webserv`
- “Helper daemons” in Python that accept the real client connections

### Legal (subject quotes the use cases)

1. **CGI scripts** run with `fork` + `execve`  
   Subject: CGI like PHP or **Python**; support at least one CGI.  
   → `www/cgi-bin/hello.py` is a **child program**, not the server.

2. **External tests**  
   Subject: write tests in a suitable language such as **Python** or Golang.  
   → `tests/test_smoke.py` is a **client**, not compiled into `webserv`.

**Defense line:** “The server is C++98. Python is only CGI payload / external tester, as the subject allows.”

---

## 3. Allowed system / C API (whitelist)

Use these for OS work (plus normal **C++98 STL** / iostreams / strings / containers):

`execve`, `pipe`, `strerror`, `gai_strerror`, `errno`, `dup`, `dup2`, `fork`, `socketpair`,  
`htons`, `htonl`, `ntohs`, `ntohl`,  
`select`, `poll`, `epoll_create`, `epoll_ctl`, `epoll_wait`,  
`kqueue`, `kevent`,  
`socket`, `accept`, `listen`, `send`, `recv`,  
`chdir`, `bind`, `connect`,  
`getaddrinfo`, `freeaddrinfo`, `setsockopt`, `getsockname`, `getprotobyname`,  
`fcntl`, `close`, `read`, `write`,  
`waitpid`, `kill`, `signal`,  
`access`, `stat`, `open`, `opendir`, `readdir`, `closedir`.

### Commonly forbidden in practice (not on the list)

| Temptation | Why not |
|---|---|
| `pthread_*`, `std::thread` | Not whitelisted; also fights “one poll loop” design |
| Boost / libevent / asio | External libs forbidden |
| `ioctl` | Not listed |
| `mmap` / `sendfile` | Not listed (don’t rely on them) |
| `inet_pton` / `inet_ntop` / `htons` alternatives not listed | Prefer `getaddrinfo` + listed helpers |
| Extra process managers | `fork` only for CGI |

If unsure: **do not call it**. Prefer STL + whitelist.

### `fcntl` note (macOS)

On macOS, subject restricts `fcntl` flags to: `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC`.  
On Linux you still need non-blocking sockets/pipes; use the allowed flags only.

### `errno` note

- `errno` appears in the whitelist (e.g. with `strerror` / setup errors).
- **After a read or write**, checking `errno` to *adjust server behaviour* is **strictly forbidden**.
- Practical approach: only I/O when poll says ready; treat `0` as close; on failure after readiness, close/cleanup without an errno-driven retry state machine.

---

## 4. I/O model (grade-0 zone) — owner: kebris-c

| Rule | Detail |
|---|---|
| Non-blocking always | Clients must not stall the server |
| **One** multiplexor | Single `poll` / `epoll` / `select` / `kqueue` for **all** client↔server I/O, **listen included** |
| Read **and** write | Multiplexor must monitor both directions as needed |
| No naked socket/pipe I/O | Never `read`/`recv`/`write`/`send` on sockets/pipes without prior readiness |
| Disk files exempt | Regular file `read`/`write` need no poll readiness |
| No hang forever | Timeouts / limits on slow clients |
| `fork` only for CGI | No prefork workers for HTTP |
| Cannot `execve` another web server | No outsourcing to nginx/apache |

Pipes used for CGI **are** “I/O that can wait” → non-blocking + same poll set.

---

## 5. HTTP / config features — owner: kmarrero (wired by kebris-c)

Mandatory behaviours:

- Config file argument (or default path)
- Compatible with a real browser
- Accurate status codes
- Default error pages if none configured
- Serve a full static website
- Client file upload
- Methods: at least **GET**, **POST**, **DELETE**
- Stress: remain available
- Listen on **multiple ports** / sites from config

### Config (IV.3) must be able to express

- All `interface:port` listen pairs
- Default error pages
- Max client body size
- Per-route (no regex required):
  - allowed methods
  - HTTP redirection
  - root / file lookup mapping (subject `/kapouet` example)
  - autoindex on/off
  - index file for directories
  - upload storage location
  - CGI by extension (+ interpreter path in practice)

Virtual hosts: **out of scope** (optional if you want).

### CGI specifics (subject)

- Unchunk request body before giving it to CGI
- CGI sees EOF as end of body
- If CGI omits `Content-Length`, EOF ends CGI output
- Run CGI in the correct working directory
- At least one CGI type (Python or php-cgi, etc.)

---

## 6. README (Ch. V) — must stay true

1. First line italic:  
   `*This project has been created as part of the 42 curriculum by kebris-c, kmarrero.*`
2. **Description**
3. **Instructions** (build/run)
4. **Resources** + how AI was used
5. English

Do not let implementation drift make the README lie.

---

## 7. Bonus (ignored until mandatory is solid)

- Cookies + session management (+ simple examples)
- Multiple CGI types  

Bonus is skipped by evaluators if mandatory fails.

---

## 8. Evaluation / defense posture

Expect:

- Browser checks against your configs
- Method / upload / CGI / multi-port demos
- Possible small live code change to prove understanding
- Questions on poll readiness, why no threads, how CGI pipes are registered, how chunked bodies are unchunked

Both partners must explain **both** planes. Ownership is for shipping, not for “I never looked at that file.”

### Suggested live demos (prepare)

```bash
make
./webserv configs/default.conf
# browser: http://127.0.0.1:8080/  and  :8081/
curl -v http://127.0.0.1:8080/
curl -v -X DELETE ...
curl -v -F file=@... http://127.0.0.1:8080/upload
curl -v 'http://127.0.0.1:8080/cgi-bin/hello.py?x=1'
# stress: parallel curls / partner Python tests
```

Compare doubtful behaviours with **nginx** + **telnet**/**nc** as the subject suggests.

---

## 9. Skeleton audit

| Artifact | Role | Compliance |
|---|---|---|
| `src/`, `include/` | C++98 server | Required |
| `Makefile` (`CXX=c++`) | Build | Required |
| `configs/` | Config samples | Required |
| `www/` | Static + errors + CGI script | Needed for eval |
| `www/cgi-bin/*.py` | CGI payload | Allowed |
| `tests/*.py` | External tester | Allowed |
| Guidance `.md` | Learning only | Extra |
| Threads / Boost / Python server | — | Forbidden; do not add |

---

## 10. Quick “should we use X?” decision

```text
Is X C++98 STL?                     -> OK
Is X on the function whitelist?     -> OK
Is X a .py CGI script or test client? -> OK (not in the binary)
Is X pthread/Boost/extra lib/sendfile/ioctl? -> NO
Does X imply a second I/O wait loop or blocking recv in a worker? -> NO
```
