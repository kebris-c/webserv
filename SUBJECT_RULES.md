# Subject rules — allowed vs forbidden (v24.0)

[Certain] Re-read from the official PDF. This file is the team cheat-sheet so nobody confuses **CGI/test Python** with the **C++98 server**.

## What the program is

| Item | Subject |
|---|---|
| Language of **webserv** | **C++98 only** |
| Binary | `./webserv [configuration file]` |
| Build | `c++` + `-Wall -Wextra -Werror` (must still build with `-std=c++98`) |
| Libraries | **No** external libs, **no Boost** |
| Submit (core) | `Makefile`, `*.{h,hpp}`, `*.cpp`, `*.tpp`, `*.ipp`, configuration files |

The HTTP server logic in `src/` / `include/` must be C++. Full stop.

## Where Python is allowed (and why you saw it)

The subject **explicitly** allows Python in two non-server roles:

1. **CGI scripts** executed by the server via `fork` + `execve`  
   > “CGI (like PHP, or Python, and so forth)”  
   > “at least one CGI (php-CGI, Python, and so forth)”  
   → `www/cgi-bin/hello.py` is a **CGI program**, not the server.

2. **External tests** written outside the binary  
   > “Write your tests in a more suitable language, such as **Python** or Golang…”  
   → `tests/test_smoke.py` is a **client/tester**, not linked into `webserv`.

If Python appears inside `src/*.cpp` as the server implementation → **illegal**.

## Allowed system / C API (whitelist)

Only these (plus normal C++98 standard library) for the server’s system work:

`execve`, `pipe`, `strerror`, `gai_strerror`, `errno`, `dup`, `dup2`, `fork`, `socketpair`, `htons`, `htonl`, `ntohs`, `ntohl`, `select`, `poll`, `epoll_*`, `kqueue`/`kevent`, `socket`, `accept`, `listen`, `send`, `recv`, `chdir`, `bind`, `connect`, `getaddrinfo`, `freeaddrinfo`, `setsockopt`, `getsockname`, `getprotobyname`, `fcntl`, `close`, `read`, `write`, `waitpid`, `kill`, `signal`, `access`, `stat`, `open`, `opendir`, `readdir`, `closedir`.

If it is not on that list and not C++98 STL / obvious freestanding C++ — **do not use it** (common traps: `pthread_*`, `std::thread`, `boost::*`, `ioctl`, random networking helpers).

## Hard forbidden / grade-0 behaviours

| Rule | Meaning |
|---|---|
| No crash / unexpected terminate | Even OOM paths should fail cleanly if possible |
| Cannot `execve` another web server | No nginx/apache as your “implementation” |
| Non-blocking always | Sockets/pipes non-blocking |
| **One** `poll`/`epoll`/`select`/`kqueue` for client↔server I/O (listen included) | No second loop, no thread-per-client I/O model |
| Poll monitors **read and write** | `POLLIN` + `POLLOUT` as needed |
| Never `read`/`recv`/`write`/`send` on sockets/pipes without prior readiness | Disk files exempt |
| No using `errno` to adjust behaviour **after** read/write | Design around readiness + return values |
| Request must not hang forever | Timeouts |
| `fork` **only** for CGI | Not for connection workers |
| Accurate HTTP status codes | Not optional |
| Default error pages | If config omits them |
| GET + POST + DELETE | Minimum methods |
| File upload | Required |
| Static website | Required |
| Multiple listen ports | Required |
| macOS `fcntl` | Only `F_SETFL`, `O_NONBLOCK`, `FD_CLOEXEC` if on macOS |

## Config must support (IV.3)

- `interface:port` listens (multiple sites/ports)
- Default error pages
- `client_max_body_size` (max request body)
- Per-route: methods, redirect, root mapping, autoindex, index file, upload store, CGI by extension

Virtual hosts: **out of scope** (allowed if you want, not required).

## Makefile note in this repo

Subject wording: compile with **`c++`**. The Makefile uses `CXX = c++`. If a broken campus/image symlink makes `c++` unusable, override locally with `make CXX=g++` — do not silently switch the project away from the subject compiler name without a comment.

## Skeleton audit (this repository)

| Artifact | OK? | Role |
|---|---|---|
| `src/*.cpp`, `include/*.hpp` | Yes | C++98 server stubs |
| `Makefile` | Yes (use `c++`) | Required |
| `configs/*.conf` | Yes | Required config samples |
| `www/**` | Yes | Static site + error pages + CGI script assets |
| `www/cgi-bin/hello.py` | Yes | CGI **payload**, exec’d by C++ server |
| `tests/test_smoke.py` | Yes | External tester language allowed by subject |
| `KEBRIS-C.md` / `KMARRERO.md` | Extra | Guidance only (not the server) |
| Boost / threads / Python server | **Absent / forbidden** | Do not add |

## Bonus (not started)

Only if mandatory is perfect: cookies/sessions; multiple CGI types.
