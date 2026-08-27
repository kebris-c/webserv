# kebris-c — manual del plano de I/O y defensa de webserv

Este documento explica el código real del repositorio. No sustituye leer el subject,
los manuales ni revisar cada línea. Su objetivo es que puedas reconstruir el flujo de
memoria y descriptores durante una defensa sin responder “eso lo hizo la IA”.

**Sincronizado con el working tree de `develop` el 2026-08-27.** Incluye los últimos
límites de recursos, deadlines por fase y terminación/reaping CGI no bloqueante.

Referencias:

- Norma principal: [`en.subject.pdf`](en.subject.pdf), versión 24.1.
- Mapa de restricciones: [`SUBJECT_RULES.md`](SUBJECT_RULES.md).
- Contrato entre propietarios: [`docs/WORK_SPLIT.md`](docs/WORK_SPLIT.md).
- Ledger de seams/workarounds: [`docs/README.md`](docs/README.md).
- Plano HTTP/config de kmarrero: [`KMARRERO.md`](KMARRERO.md).

---

## 1. Estado real: qué está hecho y qué no

### Implementado en el plano kebris-c

- Creación de listeners TCP IPv4 con `getaddrinfo`, `socket`, `setsockopt`,
  `bind` y `listen`.
- Listeners y clientes en modo no bloqueante.
- Un único `poll()` para listeners, clientes y pipes CGI.
- Varios `host:port`, con relación `listen fd -> ServerConfig`.
- Buffer de lectura por conexión.
- Buffer de escritura con offset para `send()` parcial.
- Backpressure básico: se deja de pedir `POLLIN` si hay 1 MiB pendiente de envío.
- Limpieza de desconexiones y errores de fd.
- Timeout por inactividad y deadline para fases de headers/CGI.
- Pipes CGI, `fork`, `dup2`, `chdir`, `execve` y `waitpid(WNOHANG)`.
- Cierre en el hijo de los listeners, clientes y pipes heredados.
- Peer IPv4 en `accept` → `Connection::remoteAddr()` → `prepareCgi`.
- Parada limpia mediante `SIGINT`/`SIGTERM`: el handler solo activa un flag.
- Pausa de admisión tras fallo de `accept()` para evitar busy-loop por falta de fds.
- Limpieza RAII simple: cada propietario cierra sus propios fds.
- Copia deshabilitada en clases que poseen fds o punteros.

### Workarounds temporales

Busca el bloque destacado `TODO: This is a workaround`:

1. `src/main.cpp`: si `Config::load()` falla, crea puertos 8080 y 8081.
2. `src/Server.cpp`: en ese modo devuelve por TCP los mismos bytes recibidos.
3. `src/HttpHandler.cpp`: `prepareCgi()` y `parseCgiOutput()` son stubs de
   integración.

El modo actual demuestra transporte TCP; no demuestra HTTP.

### Bloqueado por la parte kmarrero

- `Config::load()` debe entregar configuraciones reales.
- `Request::parse()` debe consumir bytes incrementalmente y deschunkear.
- `Router::match()` debe resolver location y path.
- `HttpHandler::handle()` debe producir GET/POST/DELETE.
- `HttpHandler::prepareCgi()` debe decidir CGI y construir path/env.
- `HttpHandler::parseCgiOutput()` debe convertir stdout CGI en `Response`.
- Debe usar `Connection::remoteAddr()` / el parámetro `remoteAddr` de
  `prepareCgi` al construir `REMOTE_ADDR=` (ya capturado en `accept`).
- `LocationConfig` debe poder representar el `client_max_body_size` por location
  usado en `configs/default.conf`; ahora solo existe en `ServerConfig`.
- El límite de 16 MiB protege `readBuf` y stdout CGI, pero el tamaño efectivo del body
  y de una respuesta estática debe venir del plano config/HTTP.

No declares terminada tu parte integrada hasta eliminar los tres workarounds y probar
el camino HTTP real.

---

## 2. Partes peligrosas (grade-0 / descalificativos / negativos)

Estudia esto **antes** del resto del manual. En defensa debes señalar el archivo
y la función, no solo recitar la norma. Ledger de seams: [`docs/README.md`](docs/README.md).

### 2.1 Grade-0 / “non-functional” (subject — plano I/O)

Si incumples cualquiera de estas, la evaluación puede cortar en **0** aunque HTTP
esté perfecto. Tocan **tu** código:

| Riesgo subject | Qué prohíbe / exige | Dónde lo cumples hoy | Cómo fallarías |
|---|---|---|---|
| Un solo multiplexor | Un `poll` (o equivalente) para **toda** la I/O cliente↔servidor, **listen incluido** | Solo `::poll` en `Server::run()` | Segundo `poll`/`select`/`epoll` en CGI, accept loop, o “helper thread” |
| Readiness antes de I/O | Nunca `recv`/`send`/`read`/`write` en socket/pipe sin readiness previa | `recv`→`_onReadable`, `send`→`_onWritable`, CGI `read`/`write`→`_onCgiEvent` | Leer pipe CGI “porque el hijo ya murió”; `recv` en bucle sin volver a `poll` |
| Monitorizar lectura **y** escritura | El mismo `poll` debe poder pedir `POLLIN` y `POLLOUT` | Construcción de `pollFds` en `run()` | Solo `POLLIN` y asumir que `send` siempre cabe |
| Non-blocking siempre | FDs de red/pipes del padre en `O_NONBLOCK` | `Socket::setNonBlocking`, pipes padre en `CgiProcess::start` | Listener o pipe bloqueante → un cliente congela el proceso |
| No colgar requests | Ninguna request indefinida | Idle 60s + deadline de fase 120s en `_checkTimeouts` | Sin timeout; o `waitpid` bloqueante en el event loop |
| `fork` solo CGI | Ningún worker prefork HTTP | Único `fork` en `CgiProcess::start` | `fork` para aceptar clientes o “paralelizar” handlers |
| No `execve` otro web server | El hijo ejecuta el CGI configurado | `execve(loc.cgiPass, ...)` | `execve("nginx")` / proxy a apache |
| No crashear | Ni OOM ni señales tontas tumben el proceso | `try/catch` en `main`/`run`, límites de buffer, `SIGPIPE` ignorado en servidor | Excepción sin capturar; `SIGPIPE` mata al `send` |
| C++98 / whitelist | Sin Boost/threads; solo syscalls de la lista | Build `-std=c++98`; sin `inet_ntop` (formateamos IPv4 a mano) | `pthread`, lib externa, syscall no listada |

Matiz: “un único poll” = **un multiplexor y un punto arquitectónico**. Se llama una vez
**por iteración**, no una vez en toda la vida del proceso.

Los **ficheros de disco** regulares no requieren readiness. Los **pipes CGI sí**.

Después de `recv`/`send`/`read`/`write` **no** uses `errno` para decidir el
comportamiento (cerrar/reintentar/cambiar estado). Sí puedes mirar `errno == EINTR`
tras `poll` o `waitpid(WNOHANG)`.

### 2.2 Negativos graves en eval (no siempre “cero” escrito, pero te hunden)

| Problema | Por qué duele | Mitigación en tu plano |
|---|---|---|
| Servidor “disponible” que en realidad busy-loopea | `POLLOUT` siempre listo; `accept` falla por EMFILE y el listener sigue en `POLLIN` | `POLLOUT` solo con bytes pendientes; pausa de accept / `events==0` al tope |
| Un CGI lento congela a todos | `waitpid(..., 0)` o `read` bloqueante del pipe | Solo `WNOHANG`; pipes en el mismo `poll` |
| Cliente desconectado + CGI zombie | Cerrar mal fds/pids | `shutdown` + `CGI_TERMINATING` + reap no bloqueante |
| Partial `send` perdido | Borrar `writeBuf` tras el primer `send` corto | Offset `_bytesSent` |
| Memoria ilimitada desde red | `readBuf` / stdout CGI sin tope | `MAX_READ_BUFFER` / `MAX_CGI_OUTPUT` / `MAX_CONNECTIONS` |
| Defensa: “eso lo hizo la IA / mi compañero” | Evalúa comprensión | Debes narrar fd → evento → syscall → transición de estado |

### 2.3 Trampas que ya corregimos (repásalas en el código)

Memoriza la **razón**, no solo el parche:

1. **`waitpid` bloqueante** en destructor/error → congelaba el único hilo del servidor.
2. **Cerrar el client fd** mientras el mapa aún usaba ese número y el CGI vivía → riesgo de reutilizar fd. Solución: `shutdown` + reap, luego `close`.
3. **Lifetime absoluto de 120s** en cualquier actividad → cortaba subidas/CGI legítimos. Ahora: idle + deadline **por fase** (headers/CGI).
4. **`new CgiProcess` sin ownership** si `start` fallaba → leak. Ahora: `attachCgi` antes de `start`.
5. **Hijo heredando listeners/clientes/pipes** → EOF que nunca llega. Cierre de `inheritedFds` en el hijo.
6. **`std::exit` tras `dup2` + buffers C++** → riesgo de vaciar stdout del padre al pipe. Flush antes de `fork`; `SIGPIPE` restaurado en el hijo.

### 2.4 Mini-checklist offline (5 minutos antes de defensa)

```bash
rg '::poll\\(' src          # debe ser uno: Server.cpp
rg '::(recv|send)\\(' src   # solo Server callbacks de cliente
rg '::(read|write)\\(' src  # CGI pipes tras _onCgiEvent (no sockets)
rg '::fork\\(' src          # solo CgiProcess::start
rg 'errno' src              # no ramificar tras recv/send/read/write
```

Para cada match: ¿qué `revents` lo autorizó? ¿quién posee el fd? ¿puede bloquear el loop?

Si puedes responder eso en voz alta con el PDF abierto, tu plano I/O está defendible
aunque kmarrero aún no haya terminado HTTP.

---

## 3. Mapa de responsabilidades

### `Socket`

Posee exclusivamente un listener fd.

- Lo crea y configura.
- Acepta un cliente cuando el listener está listo.
- Devuelve el nuevo client fd a `Server`.
- No recibe ni envía datos HTTP.

### `Connection`

Posee exclusivamente un client fd y su estado.

- Buffer de entrada.
- Buffer de salida y offset enviado.
- `Request` incremental.
- Índice del `ServerConfig` que aceptó al cliente.
- Inicio de la fase actual y última actividad.
- `CgiProcess*` opcional durante procesamiento CGI.

### `Server`

Posee todos los listeners y conexiones.

- Reconstruye el conjunto de `pollfd` en cada iteración.
- Es el único que decide cuándo puede hacerse I/O.
- Despacha eventos sin interpretar manualmente HTTP.
- Convierte lifecycle de conexión en intereses `POLLIN/POLLOUT`.

### `CgiProcess`

Posee el proceso hijo y los dos extremos de pipe del padre.

- Escribe el body ya deschunkeado hacia stdin del CGI.
- Cierra stdin para producir EOF.
- Recoge stdout hasta EOF.
- Reapea sin bloquear.
- No decide rutas, variables CGI ni semántica de la respuesta.

---

## 4. Flujo completo desde `main`

```text
main
  -> Config::load(path)
  -> temporalmente: si falla, crea 127.0.0.1:8080 y :8081 + modo echo
  -> Server::configure(vector<ServerConfig>)
       -> copia configs
       -> _setupListeners()
  -> Server::run()
       -> reconstruye pollfds
       -> poll()
       -> despacha listeners/clientes/CGI
       -> reapea CGI
       -> elimina timeouts
```

`main` usa un function-try-block. Si una asignación STL falla o aparece otra excepción
no controlada, retorna error en lugar de terminar mediante `std::terminate`.

`Server::run()` instala handlers para `SIGINT` y `SIGTERM`. El handler no cierra fds,
no reserva memoria y no toca contenedores: solo asigna un `volatile sig_atomic_t`.
El bucle detecta el flag, retorna normalmente y los destructores liberan conexiones,
listeners, pipes y procesos CGI. `SIGPIPE` se ignora en el servidor, pero el hijo CGI
lo restaura antes de `execve`.

El workaround de configuración debe desaparecer. En la versión final, un config
inválido debe producir error, no arrancar silenciosamente un echo server.

---

## 5. Cómo nace un listener: `Socket::listenOn`

Orden exacto:

```text
validar puerto
cerrar listener anterior si existía
crear hints IPv4/TCP
convertir puerto a string con ostringstream
getaddrinfo(host, port)
por cada dirección:
  socket()
  setsockopt(SO_REUSEADDR)
  bind()
  si falla: close() y probar siguiente
freeaddrinfo()
listen(backlog=128)
fcntl(F_SETFL, O_NONBLOCK)
guardar fd en _fd
```

### Por qué cada llamada existe

- `getaddrinfo`: genera el `sockaddr` requerido por `bind`; evita conversión manual.
- `AF_INET`: limita esta versión a IPv4. El subject no exige IPv6.
- `SOCK_STREAM`: solicita TCP, un stream de bytes fiable sin fronteras de mensajes.
- `SO_REUSEADDR`: permite reiniciar el servidor sin esperar estados TCP residuales.
- `bind`: asocia el fd con una interfaz y puerto locales.
- `listen`: convierte el socket en listener y crea una cola de conexiones pendientes.
- `O_NONBLOCK`: impide que una operación inesperada detenga todo el proceso.

`Socket` no es copiable. Dos objetos poseyendo el mismo fd provocarían doble `close()`.

### `acceptClient`

Solo se llama después de que el listener tenga `POLLIN`.

```text
clientFd = accept(listenerFd)
si falla -> -1
poner clientFd en O_NONBLOCK
devolver clientFd
```

`accept()` crea un fd nuevo. El listener sigue abierto. Desde ese momento
`Connection` será el propietario del client fd.

Limitación resuelta en transporte: `accept` rellena un string IPv4 dotted-quad
(sin `inet_ntop`, fuera de whitelist) y `Connection::remoteAddr()` lo expone.
`Server` lo pasa a `HttpHandler::prepareCgi(..., remoteAddr, ...)`. kmarrero debe
escribir `REMOTE_ADDR=<ese string>` en el entorno CGI.

---

## 6. Estado y memoria de una `Connection`

Los estados son mutuamente exclusivos:

```text
CONN_READING_REQUEST
  -> CONN_PROCESSING       si arranca CGI
  -> CONN_WRITING_RESPONSE si existe respuesta normal
  -> CONN_CLOSING          si llega EOF pero quedan bytes por enviar
```

No conviene convertir estos estados en bit flags: una conexión no debe estar
simultáneamente leyendo una request y procesando CGI. Los bitmasks sí son apropiados
para `poll.events`, donde `POLLIN | POLLOUT` puede representar intereses simultáneos.

### Entrada

`readBuf` acumula bytes TCP. TCP no conserva paquetes de aplicación:

- una request puede llegar en muchos `recv`;
- varios elementos HTTP pueden llegar en un `recv`;
- “socket legible” no significa “request completa”.

Por eso `Request::parse(readBuf)` debe ser incremental.

### Salida parcial

`send()` puede aceptar menos bytes de los solicitados.

```text
remaining = writeBuf.size - bytesSent
send(writeBuf + bytesSent, remaining)
bytesSent += resultado
si bytesSent == writeBuf.size:
  limpiar buffer y offset
```

Nunca elimines el buffer completo tras el primer `send`. Perderías el sufijo no enviado.

### Ownership

- `Connection::~Connection()` elimina primero el CGI asociado.
- Después cierra el client fd.
- `Server::_closeConnection()` borra el objeto del mapa; no cierra el fd por segunda vez.

---

## 7. El bucle único: `Server::run`

En cada iteración se crea una fotografía de intereses.

### 7.1 Listeners

Cada listener se registra con:

```text
fd = listener.fd
events = POLLIN
```

Un listener no necesita `POLLOUT`.

### 7.2 Clientes

- `POLLIN` si está leyendo y el output pendiente no supera el high-water mark.
- `POLLOUT` únicamente si hay bytes pendientes.
- Un fd con `events == 0` puede seguir informando `POLLERR/POLLHUP/POLLNVAL`.

No se debe activar `POLLOUT` permanentemente: normalmente siempre está listo y
provocaría un busy loop consumiendo CPU.

### 7.3 Pipes CGI

- Pipe padre -> CGI: `POLLOUT` mientras quede body.
- Pipe CGI -> padre: `POLLIN` mientras stdout siga abierto.

Son entradas del mismo vector y llegan a la misma llamada `poll()`.

### 7.4 Despacho

Orden:

```text
si fd es listener:
  aceptar
si fd es pipe CGI:
  mover body/stdout
si fd es cliente:
  error inválido -> cerrar
  POLLIN -> recibir
  POLLOUT -> enviar
  POLLHUP -> cerrar o drenar output
después de eventos:
  waitpid(WNOHANG) para CGIs
  barrido de timeouts
```

Después de cada callback se vuelve a comprobar si la conexión existe. Un callback
puede cerrarla, y usar el puntero anterior sería use-after-free.

### Errores

- `POLLERR`/`POLLNVAL` de cliente: cerrar la conexión.
- `POLLERR`/`POLLNVAL` de CGI: marcar el CGI fallido, terminarlo sin bloquear y
  encolar 500 mediante `_checkCgiProcesses()`.
- `POLLHUP`: el peer cerró su dirección; si hay output pendiente se intenta drenarlo.
- `recv == 0`: EOF del cliente.
- resultado negativo de `recv/send/read/write`: cerrar/fallar sin consultar `errno`.
- `poll < 0`: se permite comprobar `errno == EINTR` porque la prohibición del subject
  se aplica después de operaciones de lectura/escritura.

---

## 8. Camino de lectura

`_onReadable(fd)` solo entra desde un `POLLIN`.

```text
recv una vez hasta WEBSERV_RECV_CHUNK
si n > 0:
  append a readBuf
  touch timeout idle
  si workaround echo:
    copiar entrada a salida
  si HTTP real:
    Request::parse(readBuf)
    si incompleta: volver al poll
    si error: Response de error
    si completa:
      Router::match
      HttpHandler::prepareCgi
      CGI -> CONN_PROCESSING
      normal -> HttpHandler::handle -> CONN_WRITING_RESPONSE
si n == 0:
  cerrar, o marcar CLOSING si queda output
si n < 0:
  cerrar
```

Se hace un solo `recv` por evento. Como `poll` es level-triggered, si quedan bytes el fd
volverá a aparecer legible. Esto mantiene justo el reparto entre clientes y evita que
uno monopolice la iteración.

---

## 9. Camino de escritura

`_onWritable(fd)` solo entra desde un `POLLOUT`.

1. Calcula bytes restantes usando el offset.
2. Hace un `send`.
3. Avanza exclusivamente por el resultado positivo.
4. Actualiza actividad.
5. Al terminar:
   - echo vuelve a lectura;
   - respuesta HTTP cierra la conexión;
   - `CONN_CLOSING` drena y cierra.

La implementación final cierra tras una respuesta. Keep-alive no es obligatorio y
añadirlo implicaría resetear `Request`, límites temporales y posiblemente procesar
pipelining.

---

## 10. Backpressure y límites temporales

### Backpressure

Si el cliente no lee, `send()` acabará siendo parcial y quedará output pendiente.
Cuando ese pendiente llega a 1 MiB, el servidor deja de pedir `POLLIN` para esa
conexión. Así el cliente no puede hacer crecer memoria sin límite mientras bloquea su
propia salida.

Además existen límites defensivos propios del transporte:

- máximo de 1024 conexiones aceptadas simultáneamente;
- máximo de 16 MiB en `readBuf`;
- máximo de 16 MiB de stdout acumulado por CGI;
- capacidad de `pollFds` reservada según ese máximo para que un cliente remoto no
  fuerce realocaciones continuas del multiplexor.

Al alcanzar `MAX_CONNECTIONS`, los listeners permanecen en el mismo `poll` pero con
`events == 0` hasta que haya capacidad. Si `accept()` falla —incluido agotamiento de
fds— se pausa `POLLIN` de listeners durante un segundo. Así la readiness persistente
no convierte el event loop en un bucle de CPU.

El parser HTTP también debe aplicar límites de headers y `client_max_body_size`; esa
semántica pertenece a kmarrero.

### Dos tiempos diferentes

- Idle timeout: 60 s desde el último progreso exitoso.
- Deadline de fase: 120 s para headers y procesamiento CGI, aunque haya actividad.

Solo idle no basta: un slowloris puede enviar un byte cada 59 segundos para vivir para
siempre. El deadline de headers corta esa estrategia. El body queda gobernado por
progreso idle y por `client_max_body_size`, evitando truncar una subida legítima que
sigue avanzando.

Durante CGI, progreso en pipes actualiza actividad, pero el deadline CGI continúa
aplicándose. Si expira o el cliente desaparece, `shutdown(SHUT_RDWR)` desactiva el
tráfico sin liberar todavía la clave fd, se envía `SIGKILL` al CGI y la `Connection`
se conserva hasta reaping no bloqueante. Después se cierra definitivamente el fd.

---

## 11. Flujo CGI paso a paso

### 11.1 Frontera HTTP/CGI

kmarrero debe:

1. Determinar si la location y extensión requieren CGI.
2. Entregar `scriptPath`.
3. Construir variables de entorno.
4. Deschunkear el body antes de entregarlo.
5. Convertir stdout CGI en una `Response`.

kebris-c no debe interpretar headers CGI ni decidir rutas.

Estados de `CgiProcess`:

- `CGI_INIT`: todavía no existe un hijo activo.
- `CGI_RUNNING`: hijo y transporte asíncrono activos.
- `CGI_DONE`: hijo reapeado con éxito y stdout cerrado.
- `CGI_FAILED`: fallo de pipes, ejecución, salida o exit status.
- `CGI_TERMINATING`: se envió `SIGKILL`, pero el PID sigue siendo propiedad del
  servidor hasta que `waitpid(WNOHANG)` lo recupere.

### 11.2 Preparación antes de `fork`

`CgiProcess::start()`:

1. Valida intérprete y script.
2. Convierte `vector<string>` de entorno a array terminado en `NULL`.
3. Separa directorio y nombre del script.
4. Crea pipe de stdin.
5. Crea pipe de stdout.
6. Pone los extremos que conservará el padre en `O_NONBLOCK`.
7. Vacía `cout/cerr` para que el hijo no herede datos pendientes de los streams.
8. Hace `fork`.

Todo lo que puede reservar memoria se intenta antes del hijo. Después de `fork`, el
hijo hace un camino corto de syscalls hasta `execve`.

### 11.3 Hijo

```text
cerrar extremos del padre
cerrar listeners, clientes y otros pipes heredados
restaurar SIGPIPE a su comportamiento por defecto
dup2(pipe input read, STDIN)
dup2(pipe output write, STDOUT)
cerrar originales duplicados
chdir(directorio del script)
execve(cgiPass, argv, envp)
exit 126/127 si setup o exec fallan
```

Cerrar fds heredados es obligatorio para el lifecycle: si el hijo conserva un client
socket o un pipe, el padre puede cerrar su copia pero el kernel seguirá viendo otra
referencia abierta y no producirá EOF.

`chdir` permite que accesos relativos del CGI se resuelvan desde el directorio correcto.

### 11.4 Padre

El padre:

- cierra los extremos del hijo;
- conserva `_toChild` y `_fromChild`;
- recibe ya en `O_NONBLOCK` los extremos configurados antes de `fork`;
- si no hay body, cierra `_toChild` inmediatamente para enviar EOF;
- registra ambos extremos en el `poll` de `Server`.

### 11.5 Entrada CGI

`onPipeWritable()`:

- solo tras `POLLOUT`;
- escribe desde `_inputPos`;
- conserva el resto si la escritura fue parcial;
- al acabar cierra `_toChild`.

El cierre es parte del protocolo: para CGI, EOF señala final del body.

### 11.6 Salida CGI

`onPipeReadable()`:

- solo tras `POLLIN` o EOF indicado por `POLLHUP`;
- acumula stdout;
- al leer cero cierra `_fromChild`;
- no interpreta headers.

Si no existe `Content-Length` en la salida CGI, EOF delimita la salida según el subject.

### 11.7 Reaping

`tryReap()` usa `waitpid(pid, &status, WNOHANG)`.

- `0`: el hijo sigue vivo; volver al event loop.
- pid: terminó; comprobar exit status.
- `EINTR`: conservar el pid y reintentar en otra iteración.
- otro error: marcar CGI fallido.

Nunca se usa un `waitpid` bloqueante. `terminate()` cierra pipes, envía `SIGKILL` y
cambia a `CGI_TERMINATING`; `Server` mantiene el objeto y sigue llamando
`waitpid(WNOHANG)` hasta recuperar al hijo. Solo entonces elimina la conexión.

### 11.8 Cierre de cliente mientras CGI sigue vivo

No se libera inmediatamente el client fd porque el mapa usa ese número como clave y el
kernel podría reutilizarlo para un nuevo `accept`. El flujo es:

```text
shutdown(clientFd, SHUT_RDWR)
mantener Connection y la clave fd
terminate() -> CGI_TERMINATING
tryReap() con WNOHANG en iteraciones posteriores
cuando childActive == false:
  borrar CgiProcess
  borrar Connection
  close definitivo del client fd
```

Durante esa espera el client fd y los pipes terminados no se añaden a `poll`, evitando
un busy loop de `POLLHUP`.

---

## 12. Por qué no hay threads ni bitfields para estados

No hay threads porque no están autorizados y romperían el modelo de un solo
multiplexor.

Los estados de conexión son excluyentes y un `enum` expresa mejor el invariante. Un
bitfield permitiría combinaciones inválidas sin ahorrar memoria relevante frente a los
buffers `std::string`.

Los bitmasks sí se usan donde representan una combinación real:

```text
events |= POLLIN
events |= POLLOUT
revents & (POLLERR | POLLNVAL)
```

Ese es uso eficiente y legible de operaciones a nivel de bits.

---

## 13. Invariantes de ownership para auditar

Revisa siempre:

- Cada listener fd pertenece a exactamente un `Socket`.
- Cada client fd pertenece a exactamente una `Connection`.
- Cada pid y pipe CGI pertenece a exactamente un `CgiProcess`.
- Los mapas y vectores almacenan propietarios, no copias de objetos RAII.
- `Socket`, `Connection`, `CgiProcess` y `Server` no son copiables.
- Un error antes de transferir ownership cierra el fd local.
- Un error después de transferir ownership elimina el objeto propietario.
- El hijo CGI no conserva fds del servidor.

Pregúntate en cada rama de error:

1. ¿Quién posee el fd aquí?
2. ¿Quién lo cierra?
3. ¿Puede cerrarse dos veces?
4. ¿Puede quedar abierto tras `execve`?
5. ¿La operación I/O venía de readiness?

---

## 14. Preguntas de defensa y respuestas esperadas

### “¿Dónde garantizas que no haces recv sin poll?”

`Server::run()` llama `_onReadable()` únicamente cuando `revents & POLLIN`.
`recv()` solo existe dentro de ese callback.

### “¿Por qué monitorizas escritura?”

Una respuesta puede no caber en el send buffer. `Connection::_bytesSent` conserva el
offset y `POLLOUT` se activa mientras queden bytes.

### “¿Por qué no envías en cuanto construyes la Response?”

Porque el socket podría bloquear. Construir la cadena no es I/O de socket; enviarla sí
requiere readiness.

### “¿Qué significa recv == 0?”

EOF TCP: el peer cerró su dirección de envío. Si queda output, se marca `CLOSING` para
drenarlo; si no, se elimina la conexión.

### “¿Por qué un único poll también contiene CGI?”

Los pipes pueden bloquear igual que sockets. Un segundo bucle o lectura directa
violaría el subject y podría congelar todos los clientes.

### “¿Cómo acabas stdin del CGI?”

Después del último byte se cierra `_toChild`. El CGI usa EOF como final del body.

### “¿Por qué waitpid lleva WNOHANG?”

Sin `WNOHANG`, un CGI lento detendría el único proceso servidor.

### “¿Por qué cierras fds heredados en el hijo?”

`fork` duplica la tabla de fds. Una copia abierta impediría EOF y mantendría sockets
vivos incluso después del cierre del padre.

### “¿Por qué haces shutdown pero no close inmediato durante un CGI?”

El mapa conserva la conexión usando el número de fd como clave. Liberarlo antes de
reapear permitiría que `accept` reutilizase el número y colisionase con la entrada
antigua. `shutdown` corta el tráfico sin liberar todavía esa identidad.

### “¿Por qué hay timeout idle y deadline de fase?”

Idle corta clientes parados. El deadline corta headers lentos y CGI que prolongan
indefinidamente su actividad; no limita un body legítimo que sigue progresando.

### “¿Por qué se reconstruye el vector pollfd?”

Las conexiones y pipes cambian continuamente. Para la escala del proyecto, reconstruir
un vector simple reduce complejidad frente a mantener índices mutables.

### “¿Qué haces si accept falla repetidamente por falta de fds?”

Se desactiva temporalmente `POLLIN` de listeners durante un segundo. Mantenerlo activo
haría que `poll` devolviese readiness inmediatamente y provocaría un busy-loop.

### “¿Por qué el handler de señal solo cambia un flag?”

Los contenedores STL, `delete`, streams y la mayor parte de la biblioteca no son
async-signal-safe. `sig_atomic_t` comunica la orden; el event loop sale y hace cleanup
en flujo normal.

### “¿Por qué poll y no epoll?”

`poll` está autorizado, es portable y suficiente para la escala evaluada. Evita la
complejidad de registro y edge-triggering sin sacrificar requisitos del subject.

---

## 15. Autoauditoría técnica

### Compilación

```bash
make re
make
```

La segunda orden no debe recompilar ni relinkar.

### Localizar I/O sensible

```bash
rg '::(recv|send|read|write)\(' src
rg '::poll\(' src
rg '::fork\(' src
```

Debes justificar cada resultado y su evento previo.

### Transporte temporal

Mientras exista el workaround echo:

```bash
./webserv configs/default.conf
printf 'hello' | nc 127.0.0.1 8080
printf 'site2' | nc 127.0.0.1 8081
```

Prueba además:

- decenas de clientes concurrentes;
- payload mayor que el send buffer;
- cierre a mitad de envío;
- cliente que conecta y no envía;
- cliente que manda bytes lentamente;
- conteo de `/proc/PID/fd` antes/después.

### CGI

Prueba:

- GET sin stdin;
- POST con body y EOF;
- script inexistente;
- intérprete inexistente;
- CGI que no lee stdin;
- CGI que no termina;
- CGI que produce salida grande;
- desconexión del cliente durante CGI;
- comprobar que no quedan zombies con `ps`.

---

## 16. Checklist antes de eliminar los workarounds

- [ ] `Config::load(default.conf)` produce al menos 8080 y 8081.
- [ ] Config inválido termina con error.
- [ ] `Request::parse` funciona con fragmentos de un byte.
- [ ] Chunked queda deschunkeado.
- [ ] `Response::raw` produce status line, headers, CRLF y body correctos.
- [ ] `HttpHandler::prepareCgi` entrega path/env reales (incluye `REMOTE_ADDR=`).
- [ ] `HttpHandler::parseCgiOutput` maneja `Status`, headers y EOF.
- [ ] Browser `GET /` funciona.
- [ ] Upload y DELETE funcionan.
- [ ] CGI GET y POST funcionan desde browser/curl.
- [ ] `main` ya no arranca echo si config falla.
- [ ] No queda ningún bloque `TODO: This is a workaround`.

---

## 17. Qué debes revisar con kmarrero

Contrato mínimo:

```text
Config::load(path) -> vector<ServerConfig>
Request::parse(readBuf) -> incomplete | complete | error
Router::match(serverConfig, request) -> RouteMatch
HttpHandler::prepareCgi(req, route, remoteAddr, ...) -> path + env o false
HttpHandler::handle(...) -> Response
HttpHandler::parseCgiOutput(stdout) -> Response
Response::raw() -> bytes
```

`remoteAddr` is dotted IPv4 from `Connection::remoteAddr()` (kebris-c).

Decisiones que no deben quedar implícitas:

- Quién consume exactamente `readBuf`.
- Límite máximo de headers.
- Cuándo se aplica `clientMaxBodySize`.
- Cómo se representa “necesita CGI”.
- Qué respuesta sale si CGI falla o expira.
- Cómo se rechaza path traversal.

---

## 18. Orden de estudio recomendado

1. `include/Socket.hpp` y `src/Socket.cpp`.
2. `include/Connection.hpp` y `src/Connection.cpp`.
3. `_setupListeners()` y `_acceptNew()` en `Server.cpp`.
4. Construcción de `pollFds` en `Server::run()`.
5. Despacho de eventos.
6. `_onReadable()` y estados.
7. `_onWritable()` y offset parcial.
8. `_checkTimeouts()`.
9. `include/CgiProcess.hpp` y `CgiProcess::start()`.
10. `_onCgiEvent()` y `_checkCgiProcesses()`.
11. Interfaces públicas de kmarrero usadas desde Server.
12. Repetir preguntas de defensa sin mirar este documento.

No memorices primero `Server.cpp` de arriba abajo. Sigue el fd:

```text
config -> listener fd -> poll -> client fd -> Connection
       -> recv -> Request -> handler/CGI -> writeBuf -> send -> close
```

Si puedes narrar quién posee cada fd, qué evento autoriza cada I/O y qué transición
ocurre después, puedes auditar tu parte con criterio propio.
