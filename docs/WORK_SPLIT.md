# Work split & build order

Mandatory only. Bonus (cookies/sessions, multiple CGI types) is out of scope for now.

## Uncomfortable rule

Equal effort ≠ equal file count. **kebris-c** owns fewer files but the grade-0 traps (non-blocking I/O, single poll, never `recv`/`send` without readiness). **kmarrero** owns more surface area (HTTP, config, static site, tests). Both must be able to explain the other’s half in defense.

## Owners

### kebris-c — I/O + process plane

- Sockets: create/bind/listen/accept, multi-port
- Single `poll`/`epoll` loop (read + write)
- Non-blocking client buffers / disconnects / timeouts
- CGI child process: `fork` / `execve` / pipes registered in the same loop
- Stress resilience of the core loop

### kmarrero — HTTP + config + content plane

- Config file parser (nginx-inspired)
- Request/response parsing & building
- Location matching, methods, redirects, autoindex, uploads, DELETE
- CGI environment variables and CGI→HTTP response mapping
- Demo site, sample configs, Python tests, README polish

### both

- Day-1 interface contract (`Config` → `Server`, buffers → `Request` → `Response`)
- Integration of CGI (process vs HTTP contract)
- Defense rehearsal

## Interface contract (define before parallel coding)

```text
ConfigParser  ->  vector<ServerConfig>
Server        ->  owns listen fds + Connection map + poll set
Connection    ->  readBuf / writeBuf / parse state / timeout
Request       <-  bytes from Connection.readBuf  (kmarrero parser)
Response      ->  bytes into Connection.writeBuf (kmarrero builder)
HttpHandler   ->  uses Config + Request -> Response (or CGI job)
CgiProcess    ->  kebris runs child; kmarrero fills env + interprets output
```

Hard subject rule: socket/pipe `read`/`recv`/`write`/`send` only after poll readiness. Regular disk files are exempt.

## Phased order

| Phase | kebris-c | kmarrero |
|---|---|---|
| 1 | accept + poll + echo bytes | config lexer/parser → structs |
| 2 | multi-port + per-client buffers | GET parse + static file Response API |
| 3 | wire Request extraction from buffers | methods, redirect, autoindex, errors |
| 4 | body streaming / backpressure | POST upload + DELETE + body size |
| 5 | CGI pipes inside poll | CGI env + stdout headers/body |
| 6 | stress / slow-client / disconnect | browser demo + nginx compare + tests |

## Definition of done (mandatory)

- [ ] `./webserv configs/default.conf` serves `www/` in a real browser
- [ ] GET / POST / DELETE work per location rules
- [ ] Upload works to configured path
- [ ] At least one CGI works
- [ ] Multi-port works
- [ ] Default error pages exist
- [ ] Stress: server stays up
- [ ] No blocking I/O on sockets/pipes outside poll
