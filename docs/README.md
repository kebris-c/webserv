# Integration ledger — kebris-c ↔ kmarrero

> **INTERNAL — do not include in the evaluation Git submit.**
> Seams / workarounds / ownership for the pair only. Whole `docs/` is gitignored.
>
> **Subject (Ch. VII):** only repository content is evaluated. Intended submit
> surface: `Makefile`, sources/headers, configs, `www/`, CGI/scripts as needed,
> root `README.md` (Ch. V). Keep this ledger and the other INTERNAL guides out of
> the final push (`git rm --cached` if they were tracked earlier).

Constancia escrita de **dónde enlazan** los dos planos, qué **workarounds**
existen solo para no bloquear el transporte, y qué piezas se **pisaron** en el
terreno del otro. No sustituye [`WORK_SPLIT.md`](WORK_SPLIT.md),
[`../KEBRIS-C.md`](../KEBRIS-C.md) ni [`../KMARRERO.md`](../KMARRERO.md).

**Estado (2026-08-27):** el plano I/O de **kebris-c** está cerrado para
mandatory. Lo que falta para subject HTTP es el plano **kmarrero**, más borrar
los workarounds listados abajo.

---

## 1. Ownership (recordatorio)

| Owner | Plano | Archivos típicos |
|---|---|---|
| **kebris-c** | sockets, `poll`, `Connection`, CGI proceso/pipes/reap | `Socket.*`, `Server.*`, `Connection.*`, `CgiProcess` start/pipes |
| **kmarrero** | config, HTTP, router, handlers, CGI env/salida, www, tests | `Config.*`, `Request.*`, `Response.*`, `Router.*`, `HttpHandler.*`, `CgiProcess::buildEnv`, `configs/`, `www/`, `tests/` |
| **both** | glue | `main.cpp`, `Makefile`, `Utils.*`, README raíz |

Hard rule: `recv`/`send`/`read`/`write` en sockets/pipes **solo** tras el único
`poll` de `Server::run()`.

---

## 2. Contrato congelado (call sites reales)

`Server` ya llama esta cadena cuando **no** está en modo echo:

```text
Config::load(path) -> vector<ServerConfig>     # kmarrero — hoy stub
Server::configure / _setupListeners            # kebris-c
poll(listeners + clients + CGI pipes)          # kebris-c

recv -> Connection::readBuf
Request::parse(readBuf)                        # kmarrero — hoy stub
Router::match(server, request)                 # kmarrero — hoy stub

HttpHandler::prepareCgi(req, route, remoteAddr, scriptPath, env)
  -> false | true                              # kmarrero — workaround stub
CgiProcess::start(...)                         # kebris-c
  pipe I/O via same poll                       # kebris-c
HttpHandler::parseCgiOutput(stdout)            # kmarrero — workaround stub

HttpHandler::handle(...) -> Response           # kmarrero — hoy 501
Response::raw() -> writeBuf                    # kmarrero build / kebris-c send
```

### Datos que kebris-c ya entrega a kmarrero

| Dato | Dónde | Uso esperado |
|---|---|---|
| Bytes TCP | `Connection::readBuf()` | `Request::parse` consume el prefijo |
| Índice de config | `Connection::serverIndex()` | elegir `ServerConfig` del listener |
| Peer IPv4 | `Connection::remoteAddr()` | `REMOTE_ADDR` en CGI (`prepareCgi` ya recibe el string) |
| Body deschunkeado | `Request::body()` (cuando exista) | stdin CGI vía `CgiProcess::start` |
| Stdout CGI | `CgiProcess::output()` | `parseCgiOutput` |

`chdir` CGI: kebris-c cambia al **directorio del script** antes de `execve`.

---

## 3. Workarounds que **solo kmarrero** debe completar / borrar

Buscar el banner:

```text
// TODO: This is a workaround to allow me keep going on, must be
// changed/improved before the project end.
```

| Ubicación | Qué hace hoy | Qué debe hacer kmarrero |
|---|---|---|
| `src/main.cpp` | Si `Config::load` falla → inventa `127.0.0.1:8080`/`:8081` y activa echo | Hacer que `load()` funcione; **eliminar** el fallback (config inválido = error) |
| `src/Server.cpp` (`_onReadable`) | Con echo: copia `readBuf` → `writeBuf` | No es HTTP; desaparece al quitar echo en `main` |
| `HttpHandler::prepareCgi` | Stub: ignora args, `return false` | Decidir CGI, rellenar `scriptPath` + env (incl. `REMOTE_ADDR=` desde el parámetro) |
| `HttpHandler::parseCgiOutput` | Stub: `501` | Parsear headers CGI + body → `Response` |

Estos stubs viven en archivos de kmarrero (o glue compartido) porque el
transporte necesitaba **compilar y enlazar** sin bloquear la parte I/O. No son
features finales de kebris-c.

También quedan `TODO(kmarrero):` nativos en `Config`, `Request`, `Response`,
`Router`, `HttpHandler`, `CgiProcess::buildEnv`, `Utils` — trabajo obligatorio
suyo, no workarounds de desbloqueo.

---

## 4. Terreno pisado (disclaimers)

Piezas donde un owner tocó interfaz o archivos del otro **sin** completar la
semántica:

| Qué | Quién empujó | Por qué | Quién termina |
|---|---|---|---|
| `prepareCgi` / `parseCgiOutput` añadidos a `HttpHandler` | kebris-c | `Server` necesita un punto de decisión CGI sin hacer HTTP | kmarrero (sustituir stubs) |
| Firma `prepareCgi(..., remoteAddr, ...)` | kebris-c | Entregar peer sin ensuciar `Request` | kmarrero (usar el string en env) |
| Echo + configs hardcodeadas en `main` | kebris-c | `Config::load` siempre falla | kmarrero (`load`) + borrar echo |
| `CgiProcess::start` / poll / reap ya implementados | kebris-c | Su ownership de proceso | kmarrero solo `buildEnv` + parse salida |
| `configs/default.conf` con `client_max_body_size` **dentro** de `location /upload` | kmarrero (sample) | Struct `LocationConfig` **no** tiene ese campo aún | kmarrero (extender struct o cambiar el conf) |

kebris-c **no** implementa parser HTTP, router ni handlers “de verdad”. Si alguien
ve lógica HTTP real en `Server` fuera del workaround echo, es un error de
ownership.

---

## 5. Cierre kebris-c (checklist)

- [x] Un único `poll` para listeners, clientes y pipes CGI  
- [x] Listeners/clientes/pipes padre no bloqueantes  
- [x] Multi-port + mapa `listen fd → ServerConfig`  
- [x] Partial `send`, backpressure, límites de buffer/conexiones  
- [x] Timeouts idle + deadline de fase  
- [x] CGI fork/pipes/`chdir`/`execve`/`waitpid(WNOHANG)` + cierre sin bloquear  
- [x] Señales `SIGINT`/`SIGTERM` (solo flag)  
- [x] Backoff si `accept` falla / tope de conexiones  
- [x] Peer IPv4 en `Connection::remoteAddr()` pasado a `prepareCgi`  

Pendiente **solo** de integración conjunta: borrar banners de workaround cuando
el plano HTTP de kmarrero pase `GET /` en browser.

---

## 6. Qué falta (kmarrero — mandatory)

1. `Config::load` + validación; alinear structs con `default.conf`  
2. `Request::parse` incremental + unchunk  
3. `Response` ficheros / error pages configurables  
4. `Router` longest-prefix + path mapping / anti-`..`  
5. `HttpHandler` GET/POST/DELETE/redirect/autoindex/upload  
6. `prepareCgi` + `CgiProcess::buildEnv` + `parseCgiOutput` reales  
7. Smoke tests + demo `www/` alineados con conf  
8. Eliminar los cuatro workarounds de la tabla §3  

Guía detallada: [`../KMARRERO.md`](../KMARRERO.md) (sección *Integration status*).

---

## 7. Cómo buscar

```bash
rg 'TODO: This is a workaround' src include
rg 'TODO\\(kmarrero\\)' src include
rg 'prepareCgi|remoteAddr|enableEchoWorkaround' src include
```
