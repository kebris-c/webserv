*This project has been created as part of the 42 curriculum by kebris-c, kmarrero.*

# webserv

## Description

`webserv` is an HTTP/1.x server written in C++98. It reads an nginx-inspired
configuration file, listens on one or more `interface:port` pairs, and serves
clients through a single non-blocking `poll` loop.

The server:

- Serves a fully static website
- Supports at least **GET**, **POST**, and **DELETE**
- Allows file uploads
- Applies per-route (location) rules: methods, redirects, root, autoindex, index
file, upload path, and CGI by extension
- Runs at least one CGI (for example Python)
- Remains available under stress and does not hang requests indefinitely
- Returns accurate HTTP status codes and default error pages when none are configured

**Language boundary:** the `webserv` binary is **C++98 only**. Any `.py` file in
the tree is either a **CGI script** executed by the server or an **external test
client** — both uses allowed by the subject.


## Instructions

### Requirements

- `C++98` toolchain: compile with `c++ -Wall -Wextra -Werror -std=c++98`
- No Boost and no external libraries; stay inside the subject function whitelist
- Unix-like environment with `poll`



### Build

```bash
make
```

Useful targets: `all`, `clean`, `fclean`, `re`. The Makefile invokes `c++` as
required by the subject.

### Run

```bash
./webserv configs/default.conf
```

If no argument is provided, the program uses `configs/default.conf`.

### Demo content

- Static site root: `www/`
- Default error pages: `www/errors/`
- Example configuration: `configs/default.conf`



## Resources



### Classic references

- [RFC 9110 — HTTP Semantics](https://www.rfc-editor.org/rfc/rfc9110.html)
- [RFC 9112 — HTTP/1.1](https://www.rfc-editor.org/rfc/rfc9112.html)
- [RFC 3875 — CGI 1.1](https://datatracker.ietf.org/doc/html/rfc3875)
- [nginx Beginner’s Guide / server & location](https://nginx.org/en/docs/beginners_guide.html)
- `man 2 poll`, `man 2 socket`, `man 2 accept`, `man 2 recv`, `man 2 send`
- Compare behaviour with a local **nginx** instance and raw clients (`curl`, `telnet`)



### How AI was used

AI tools were used as assistants for:

- Clarifying subject constraints (C++98, single `poll` loop, non-blocking I/O, CGI)
- Structuring the README and organizing reference material
- Reviewing design options for sockets, connection lifecycle, timeouts, partial I/O,
and CGI process/pipe handling against the subject rules
- Suggesting external test ideas (stress, disconnects, sanitizers)

All submitted code and documentation were reviewed, understood, and owned by the
team. AI output was never treated as a substitute for reading the subject, RFCs,
manuals, or peer review.