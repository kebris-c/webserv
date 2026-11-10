# Defensa — recorrido de una request

Los dos explicáis los dos planos. El nombre dice quién lo escribió, no quién puede callarse.

Subject v24.1. Un proceso, C++98, un solo `poll`. `recv` / `send` / `read` / `write` de socket o pipe solo si ese fd salió listo. El disco no pasa por `poll`. `fork` solo para CGI. Python es el script (`www/cgi-bin/hello.py`) o un tester, no el servidor.

| Quién | Código |
|---|---|
| **kebris-c** | `Socket`, `Server`, `Connection`, proceso CGI (`start`, pipes, `fork` / `execve` / `waitpid`), señales |
| **kmarrero** | config (`Lexer`, `Parser`, `StateMachine`, `Config`), `Request*`, `Router`, `HttpHandler`, `Response`, `CgiUtils`, `CgiProcess::buildEnv`, `configs/`, `www/` |
| **los dos** | `main.cpp`, `Makefile`, `Utils` |

Arranque real: `./webserv configs/default.conf`. Sin ese argumento el programa sale. El subject acepta argumento **o** un path por defecto; aquí solo existe el argumento. `WEBSERV_DEFAULT_CONF` no se usa.

---

## No defendible — decirlo antes de que lo vean

Cada punto está en el código de ahora. No lo adornéis.

1. **Redirección muerta.** `location /old { return 301 /; }` se parsea (`redirect`, `redirectCode`). `handleRedirect` no lo llama nadie. `GET /old` entra en `handleGet` y busca un fichero. El subject pide redirección HTTP en una ruta. Con este config no sale un 301.

2. **`error_page` no se aplica.** El parser guarda `ServerConfig::errorPages`. `Response::makeError` ignora `pagePath` y manda un HTML de una línea. `www/errors/` no entra en la respuesta. El HTML mínimo cubre «si no hay página configurada». No cubre «la config define la página».

3. **`index` de la config no se lee.** `handleGet`, si el path es directorio, busca el literal `index.html`. En el sample coincide. Si cambiáis `index`, el servidor no se entera.

4. **`upload_store` no se lee.** El POST escribe en `route.fsPath`. En el sample eso cae bajo `www/uploads/` por cómo se concatena el path, no porque se use la directiva. Cambiad el directorio de `upload_store` y el fichero no irá ahí.

5. **El `root` no sigue el ejemplo del subject.** El PDF: si `/kapouet` tiene root `/tmp/www`, `/kapouet/pouic/toto/pouet` se busca en `/tmp/www/pouic/toto/pouet` (el prefijo de la location se quita). `_mapToFilesystem` hace `root + uri` entero: daría `/tmp/www/kapouet/pouic/...`. El sample «funciona» porque `location /files` + `root www` + uri `/files/readme.txt` produce `www/files/readme.txt`, que es donde está el fichero. Es el `root` de nginx, no el ejemplo del PDF. Además hay un `std::cerr` en cada match.

6. **`handle` no mira si `location` es nulo.** `prepareCgi` devuelve false si no hay location, y el `else` llama a `handle`, que desreferencia `route.location`. Con `location /` en el sample toda URI absoluta encaja y no crashea. Un server sin esa location es un null deref: el subject pone 0 si el proceso muere.

7. **`REQ_ERROR` sigue hacia el handler.** `_onReadable` solo vuelve a `poll` si falta body. Si el parse falló, igual hace match y `handle`. `errorCode()` no se convierte en la respuesta.

8. **Chunked no es incremental de verdad.** `parseChunkedBody` tira el `REQ_WAIT` de `chunkCalculations` y de `consumeTrailerInFinalLine`, y también el `REQ_COMPLETE` del chunk 0 (ahí ya se hizo `setBody`). El subject exige deschunkear antes del CGI. No lo deis por bueno sin un POST chunked visto en `curl`.

9. **`client_max_body_size` dentro de un location escribe el campo del server.** `LocationConfig` no tiene ese campo. El `2M` de `/uploads` pisa el `10M` del server. El tope existe; los dos números del sample no significan lo que parecen.

10. **Headers CGI incompletos.** `buildEnv` manda método, query, protocolo, puerto, nombre, `SCRIPT_*`, `CONTENT_LENGTH`, `CONTENT_TYPE` si venía, y `REMOTE_ADDR`. No convierte el resto de headers del cliente en `HTTP_*`. El subject dice que la request completa y los argumentos tienen que llegar al CGI.

11. **Llamadas fuera de la whitelist:** `lstat`, `unlink`, `rmdir`, `getcwd`. `stat` sí está. DELETE y el path absoluto del CGI, como están escritos, no salen de la lista. Si os lo preguntan, señalad la línea. No digáis que el subject las autoriza.

12. **Echo muerto.** `enableEchoWorkaround` y el branch de `_onReadable` siguen ahí. `main` no los enciende. No es HTTP. No lo presentéis.

`Request::header` compara el nombre tal cual (`Content-Type` ≠ `content-type`). El TODO sigue en el fuente.

---

## 1. Arranque — `main.cpp` (los dos)

`argc` tiene que ser 2. `Config::load` (kmarrero). Si falla o no hay servers, sale: no hay echo ni puertos inventados. `Server::configure` + `Server::run` (kebris).

## 2. Config — kmarrero

`Lexer` tokeniza (salta comentarios). `Config::addTransitions` registra la máquina. `StateMachine` camina hasta `END` o error. `Parser::keyword` despacha `listen`, `server_name`, `client_max_body_size`, `error_page`, `root`, `index`, `allowed_methods`, `autoindex`, `return`, `upload_store`, `cgi_extension`, `cgi_pass`.

Sale `vector<ServerConfig>`. Cada uno tiene `host`, `port`, `clientMaxBodySize`, `errorPages` y `vector<LocationConfig>`. `Socket` no ve el texto: solo `host` y `port`.

## 3. Listeners — kebris

`_setupListeners` → `Socket::listenOn` por cada server: `getaddrinfo` (IPv4), `socket`, `SO_REUSEADDR`, `bind`, `listen(128)`, `fcntl(O_NONBLOCK)`. `_listenerConfigs[fd]` guarda el índice del `ServerConfig`. Si uno falla, se cierran los que ya abrió.

`8080` y `8081` son dos fds de escucha en el mismo `poll`, no dos procesos. El índice del listener elige el sitio (`www/` o `www/site2/`).

## 4. El único `poll` — kebris, `Server::run`

Antes del bucle: `SIGPIPE` ignorado. `SIGINT` / `SIGTERM` solo ponen `g_stopRequested`. El handler no cierra fds.

Cada vuelta se rearma el array:

- listeners en `POLLIN`, salvo con ≥ 1024 clientes o si `accept` falló hace menos de 1 s
- cliente en `CONN_READING_REQUEST`: `POLLIN` solo si lo pendiente de envío es < 1 MiB
- cliente con bytes por enviar: `POLLOUT`
- CGI en `CGI_RUNNING`: stdin del hijo `POLLOUT`, stdout `POLLIN`

`poll(..., 500)`. Esos 500 ms existen para mirar timeouts e hijos sin bloquear. `EINTR` reintenta. Por fd listo: listener → `_acceptNew`; si no, `_onCgiEvent`; si no, cliente → lectura, escritura o `POLLHUP`. Al cerrar la vuelta: `_checkCgiProcesses` y `_checkTimeouts`.

El tope de 1 MiB deja de leer a un cliente al que no podemos escribir. El fallo de `accept` pausa la admisión 1 s para no girar en vacío si se acaban los fds.

## 5. Accept — kebris

`accept` no bloqueante, cliente en no bloqueante. La IPv4 del peer se calcula con `ntohl` (sin `inet_ntop`, que no está en la lista) y queda en `Connection::remoteAddr`. La conexión nace leyendo, atada al índice de ese listener.

## 6. Bytes — kebris recibe, kmarrero interpreta

`recv` de un trozo. Por encima de 16 MiB en el buffer, se cierra. `recv == 0` es peer cerrado.

`Request::parse` → `feed`: los bytes pasan a `RequestContext::buffer` y la `RStateMachine` avanza: línea (`método`, target, versión) → headers → body por `Content-Length` o chunked → `REQ_COMPLETE`, `REQ_WAIT` o `REQ_ERROR`.

Luego `Server` vacía `readBuf`: la copia ya está en el contexto. Si no está completo y no es error, vuelve a `poll`. El `RequestParser` nuevo de cada `recv` no guarda estado; el estado vive en el `Request` de la conexión.

El hueco del punto 7 de arriba está en este `if`.

## 7. Router — kmarrero

`Router::match` elige el `location` cuyo path es prefijo del URI y el más largo. Hace falta fin de string o `/` justo después, para que `/old` no capture `/oldest`. `/` pierde contra `/files`, `/uploads`, `/cgi-bin`.

`.` y `..` se rechazan (`fsPath` vacío, `ok = false`). No se resuelven.

El path que devuelve es el del punto 5 de arriba.

## 8. Respuesta — kmarrero, `HttpHandler::handle`

1. Método no permitido → 405. Si `allowed_methods` está vacío, solo pasa `GET`.
2. `GET`: `lstat`. Fichero → `setBodyFromFile` (lee el disco, `Content-Type` por extensión). Directorio → el `index.html` fijo; si no está y `autoindex` es on, `opendir` / `readdir` con el HTML escapado. Si no, 403.
3. `POST`: 413 si `body > clientMaxBodySize`. Fichero existente: lo sobrescribe (200, `File Uploaded`). Si no existe: lo crea (201). Directorio: 409.
4. `DELETE`: `unlink` o `rmdir`. Éxito → 204. Directorio no vacío → 409.
5. Otro método → 501.

`Response::setBody` pone `Content-Length`. `raw()` arma `HTTP/1.1`, reason, headers, línea en blanco, body. Leer el disco aquí es legal sin `poll`: el subject exime los ficheros regulares.

La redirección, las error pages, el `index` y el `upload_store` no entran en este camino. Están en la lista de arriba.

## 9. CGI — decisión kmarrero, proceso kebris

`prepareCgi` devuelve true solo si la request está completa, hay `cgi_pass` y extensión, el método es GET o POST y está permitido, el body cabe en el límite del server, y el path termina en la extensión (`.py`). Monta el path absoluto (`getcwd` si hace falta), llama a `buildEnv` y añade `REMOTE_ADDR`.

Si es CGI, `Server` junta los fds que el hijo no debe heredar (listeners, clientes, pipes de otros CGI) y `CgiProcess::start`:

- dos `pipe`, extremos del padre en no bloqueante
- `fork`
- hijo: cierra heredados, `dup2` a stdin/stdout, `chdir` al directorio del script, `execve(cgi_pass, [intérprete, nombre], env)`. Si falla, `exit 127`
- padre: extremo de escritura (el body) y de lectura (stdout). Body vacío → cierra stdin ya

Esos dos fds entran en el mismo `poll`. `onPipeWritable` empuja el body y cierra al terminar (el CGI ve EOF). `onPipeReadable` acumula stdout, tope 16 MiB, y en EOF cierra. `tryReap` es `waitpid(WNOHANG)`: no espera. `CGI_DONE` solo con salida 0 y el stdout ya cerrado. Si no, 500.

`_checkCgiProcesses` llama a `parseCgiOutput`: separa headers CGI y body, lee `Status` (sin `Status` y con `Location` → 302), exige `Content-Type` salvo redirect vacío, rechaza `Transfer-Encoding`, y monta el `Response`.

La conexión se queda en `CONN_PROCESSING` hasta el hijo o el timeout de fase (120 s). `terminate` mata; el reap sigue siendo `WNOHANG`.

## 10. Envío y cierre — kebris

`_onWritable`: `send` desde `bytesSent`. Un `send` corto no pierde bytes. Al vaciar, cierra. No hay varias requests en la misma conexión.

`_checkTimeouts`: 60 s sin actividad, o 120 s en headers / CGI. Si hay un CGI vivo, no se borra la conexión hasta el reap: `shutdown` del cliente y `terminate` del hijo.

`~Server` borra conexiones y listeners. Cada dueño cierra sus fds. `errno` no se usa para decidir un reintento después de `recv` / `send` / `read` / `write`: a 0 se cierra, a fallo se limpia.

---

## Orden para decirlo

1. Un proceso, un `poll`, no bloqueante, lectura y escritura a la vez.
2. Config → N listeners → el fd de escucha elige el `ServerConfig`.
3. `recv` parcial → máquina de estados → complete / wait / error.
4. Longest prefix, `..` rechazado, método, GET / POST / DELETE.
5. CGI en el mismo `poll`: `fork` + `execve`, env, stdout a HTTP.
6. `send` parcial, timeouts, la señal solo marca un flag.
7. La lista de arriba, con archivo y función, antes del `curl`.
