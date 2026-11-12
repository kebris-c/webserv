# Defensa webserv — flujo y puntos que hay que saber

Subject v24.1. Los dos debéis explicar todo. El owner indica quién implementó el bloque.

## Reglas que suspenden

- C++98, `c++ -Wall -Wextra -Werror`, sin Boost ni librerías externas.
- Un proceso y un único `poll` para listeners, clientes y pipes CGI.
- Socket o pipe no bloqueante. `recv`, `send`, `read` y `write` solo tras readiness.
- No consultar `errno` para reintentar después de esas cuatro llamadas.
- `fork` solo para CGI. El servidor nunca ejecuta otro web server.
- Una request no puede quedar colgada: límites y timeouts.
- Python solo es CGI o test externo; el servidor es C++98.

## Ownership

- **kebris-c:** `Socket`, `Server`, `Connection`, poll, buffers, timeouts, señales y proceso/pipes/reap CGI.
- **kmarrero:** config, request parser, router, handlers, response, entorno/salida CGI, configs y `www`.
- **Mutuo:** `main.cpp`, `Makefile`, utilidades, integración, tests, README y defensa.

## Flujo completo

### 1. Build y arranque — mutuo

`make` compila cada `.cpp` en `obj/`, genera dependencias `.d` y enlaza `webserv`.
Un segundo `make` no relinka. `clean`, `fclean` y `re` cumplen el subject.

`./webserv configs/default.conf`: `main` carga el config, comprueba que haya servers,
configura listeners y entra en `Server::run`. Config inválida significa error y salida.

### 2. Config — kmarrero

`Lexer` tokeniza. `StateMachine` controla bloques. `Parser` llena:

- `ServerConfig`: host, port, nombre, límite de body, error pages y locations.
- `LocationConfig`: path, root, métodos, redirect, autoindex, index, upload store, CGI y límite local.

El límite local `0` hereda el del server. `root` admite rutas relativas o absolutas seguras.
`autoindex` solo acepta `on/off`; `return` solo códigos 3xx.

### 3. Listeners — kebris-c

Por cada `host:port`, `Socket::listenOn` hace:
`getaddrinfo → socket → setsockopt → bind → listen → fcntl(O_NONBLOCK)`.

`_listenerConfigs[fd]` relaciona listener y `ServerConfig`. 8080 y 8081 son fds
distintos dentro del mismo proceso y poll.

### 4. Poll — kebris-c

Cada vuelta registra listeners y clientes en `POLLIN`, clientes con respuesta en
`POLLOUT`, stdin CGI en `POLLOUT` y stdout CGI en `POLLIN`.

`poll` espera hasta 500 ms. Después despacha únicamente fds preparados. Al final
revisa hijos CGI y timeouts. Hay máximo de conexiones, buffers, backpressure y
offset para `send` parcial.

`SIGINT`/`SIGTERM` solo cambian un `sig_atomic_t`; la limpieza ocurre fuera del handler.
`SIGPIPE` se ignora para que un cliente desconectado no mate el servidor.

### 5. Accept y Connection — kebris-c

Tras `POLLIN` del listener, `accept` crea un cliente no bloqueante. `Connection`
posee fd, buffers, offset, request incremental, config, dirección remota,
timestamps y CGI opcional.

### 6. Request — kmarrero

`Server::_onReadable` hace `recv` y entrega bytes a `Request::parse`.
Estados: request-line → headers → body → complete/error/wait.

- Headers guardados en minúsculas: lookup case-insensitive.
- `Content-Length` y chunked no pueden coexistir.
- Chunked admite fragmentación; chunk 0 finaliza y el CGI recibe body plano.
- El límite efectivo se comprueba antes de acumular: exceso → 413.
- Framing inválido → 400. `REQ_ERROR` no entra en router ni handler.

### 7. Router — kmarrero

Elige la location de prefijo más largo y rechaza segmentos `.` y `..`.
Path de disco = `root + URI sin el prefijo de location`.

Ejemplo: location `/kapouet`, root `/tmp/www`, URI `/kapouet/pouic`
produce `/tmp/www/pouic`.

### 8. Handler y Response — kmarrero

Orden: redirect → ruta válida → método permitido → GET/POST/DELETE.

- GET sirve fichero, index configurado o autoindex.
- POST escribe en `upload_store`; exceso de body → 413.
- DELETE borra fichero o directorio vacío; éxito → 204.
- Error configurado carga su HTML; si falla, usa HTML interno.

`Response::raw` construye status-line, headers, CRLF y body. `setBody` fija
`Content-Length`. Los ficheros regulares no pasan por poll: el subject los exime.

### 9. CGI — ambos

**kmarrero:** decide por extensión y construye método, query, protocolo, server,
script, length/type, `REMOTE_ADDR` y headers `HTTP_*` salvo hop-by-hop.

**kebris-c:** crea dos pipes y hace `fork`.

- Hijo: cierra heredados, `dup2`, `chdir`, `execve`.
- Padre: escribe body tras `POLLOUT`, cierra stdin para EOF y lee stdout tras `POLLIN`.
- `waitpid(WNOHANG)` reap durante servicio; al apagar se recoge cualquier hijo pendiente.

La salida CGI se valida y convierte en `Response`.

### 10. Envío y cierre — kebris-c

Tras `POLLOUT`, `send` empieza en `bytesSent`; un envío parcial avanza el offset.
Al terminar se cierra la conexión. No hay keep-alive ni pipelining.

Timeouts: 60 s de inactividad y 120 s para headers/CGI. Si el cliente muere durante
CGI, se apaga el socket, se termina el hijo y se reap.

## Demo

```bash
make
python3 tests/test_smoke.py
./webserv configs/default.conf
```

Mostrar: `/`, `/style.css`, puerto 8081, `/files/`, `/old`, `/missing`,
CGI con query, POST/GET/DELETE en `/uploads/demo.txt`, CGI chunked, 413,
request fragmentada, request sin Host y Ctrl+C.

## Contradicción del subject

El PDF exige DELETE, pero su whitelist v24.1 no contiene una primitiva de borrado.
Se conservan `unlink` y `rmdir`, el mínimo necesario para cumplir DELETE.
No hay alternativa que cumpla literalmente ambas cláusulas.

## Checklist final

- El PDF del subject no está trackeado.
- `make re`, segundo `make` sin relink y `python3 tests/test_smoke.py`.
- Sin binario, `obj/`, uploads temporales ni procesos webserv/CGI vivos.
- README en inglés; esta guía en español.
- Ambos recorréis el flujo sin decir “esa parte no es mía”.
