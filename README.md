*This project has been created as part of the 42 curriculum by kebris-c, kmarrero.*

# webserv

## Description

`webserv` is an HTTP/1.x server written in C++98. It reads an nginx-inspired configuration file, listens on one or more `interface:port` pairs, and serves clients through a **single non-blocking `poll` loop**.

The server aims to:

- Serve a fully static website
- Support at least **GET**, **POST**, and **DELETE**
- Allow file uploads
- Apply per-route (location) rules: methods, redirects, root, autoindex, index file, upload path, CGI by extension
- Run at least one CGI (for example Python)
- Remain available under stress and never hang a request indefinitely
- Return accurate HTTP status codes and default error pages

The kebris-c transport scaffold, resilience limits, and CGI process mechanics are
implemented. End-to-end HTTP/CGI remains blocked by the owned kmarrero HTTP/config
skeletons, so the executable temporarily runs a clearly marked TCP echo workaround.

**Language boundary:** `webserv` itself is **C++98 only**. Any `.py` in the tree is either a **CGI script** exec’d by the C++ server or an **external test client** — both explicitly allowed by the subject. See [`SUBJECT_RULES.md`](SUBJECT_RULES.md).

## Instructions

### Requirements

- C++98 toolchain: compiler invoked as `c++` with `-Wall -Wextra -Werror -std=c++98`
- No Boost / no external libraries; stay inside the subject function whitelist
- Unix-like environment with `poll`

### Build

```bash
make
# if `c++` is a broken clang symlink in your environment:
make CXX=g++
```

Useful targets: `all`, `clean`, `fclean`, `re`. Compiler name in the Makefile is `c++` as required by the subject.

### Run

```bash
./webserv configs/default.conf
```

If no argument is provided, the program uses `configs/default.conf`.

Until the owned config parser is implemented, a highlighted workaround starts a TCP
echo transport test on ports `8080` and `8081`; this is not an HTTP-capable release.

### Demo content

- Static site root: `www/`
- Default error pages: `www/errors/`
- Example configuration: `configs/default.conf`

### Team ownership (mandatory only — no bonus for now)

| Area | Owner | Main paths |
|---|---|---|
| I/O plane: sockets, poll loop, connections, CGI process | **kebris-c** | `include/Socket.hpp`, `Server.*`, `Connection.*`, `CgiProcess.*` |
| HTTP/config plane: parser, request/response, routes, handlers, tests, demo | **kmarrero** | `include/Config.hpp`, `Request.*`, `Response.*`, `Router.*`, `HttpHandler.*`, `configs/`, `www/`, `tests/` |
| Bootstrap / shared glue | **both** | `src/main.cpp`, `include/Webserv.hpp`, `Makefile`, this README |

See comments inside each file for **OWNER**, **GOAL**, **LEARN**, and **PSEUDOCODE** where the subject is hardest.

Per-person walkthroughs (phases, clarifications, what to study):

- [`KEBRIS-C.md`](KEBRIS-C.md) — sockets, poll loop, connections, CGI process
- [`KMARRERO.md`](KMARRERO.md) — config, HTTP, routes, handlers, demo, tests

Shared build order: [`docs/WORK_SPLIT.md`](docs/WORK_SPLIT.md).
Seams, workarounds, and cross-owner touchpoints: [`docs/README.md`](docs/README.md).

## Resources

### Classic references

- [RFC 9110 — HTTP Semantics](https://www.rfc-editor.org/rfc/rfc9110.html)
- [RFC 9112 — HTTP/1.1](https://www.rfc-editor.org/rfc/rfc9112.html)
- [RFC 3875 — CGI 1.1](https://datatracker.ietf.org/doc/html/rfc3875)
- [nginx Beginner’s Guide / server & location](https://nginx.org/en/docs/beginners_guide.html)
- `man 2 poll`, `man 7 epoll`, `man 2 socket`, `man 2 accept`, `man 2 recv`, `man 2 send`
- Compare behaviour with a local **nginx** instance and raw clients (`curl`, `telnet`)

### How AI was used

AI assisted with:

- Reading the subject and proposing an equal ownership split by prior skills
- Generating this repository skeleton, ownership notes, learning checklists, and hard-path pseudocode
- Drafting the subject-compliant README structure
- Implementing and auditing kebris-c's non-blocking sockets, connection lifecycle,
  `poll`, timeouts, partial I/O, and CGI process mechanics
- Designing stress, descriptor-lifecycle, sanitizer, and CGI tests

Both owners must review, understand, and take responsibility for every submitted line
before evaluation.
