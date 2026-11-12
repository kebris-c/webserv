# Guía de estudio — kebris-c

Complemento de [DEFENSA.md](DEFENSA.md). Sigue su orden, pero solo desarrolla lo que tienes que dominar. La semántica HTTP de kmarrero aparece únicamente donde tu código la llama.

Guía local de estudio. No forma parte del entregable.

## Contrato que debes recitar

El servidor es un solo proceso. Todos los sockets y pipes esperan en un único `poll`. Solo cuando `poll` marca un fd listo haces `accept`, `recv`, `send`, `read` o `write`. Si la llamada transfiere menos bytes de los pedidos, guardas el progreso y esperas otra vuelta. `errno` no decide un reintento después de esas cinco operaciones.

Archivos: [include/Server.hpp](../include/Server.hpp), [src/Server.cpp](../src/Server.cpp), [include/Socket.hpp](../include/Socket.hpp), [src/Socket.cpp](../src/Socket.cpp), [include/Connection.hpp](../include/Connection.hpp), [src/Connection.cpp](../src/Connection.cpp), [include/CgiProcess.hpp](../include/CgiProcess.hpp), [src/CgiProcess.cpp](../src/CgiProcess.cpp).

## 1. Arranque — mutuo, pero entras aquí

[src/main.cpp](../src/main.cpp) exige exactamente un argumento. `Config::load` es de kmarrero. Si devuelve servers, tu frontera empieza en `server.configure(servers)` y `server.run()`.

```16:57:src/main.cpp
int main(int ac, char **av)
{
    // ...
    if (!server.configure(servers))
        return (1);
    return (server.run());
}
```

Debes saber: un config inválido sale antes del bucle; un fallo de `listen` también. El proceso no continúa “a medias”.

## 2. Config — frontera con kmarrero

No parses el archivo. Recibes `vector<ServerConfig>` ya validado. De cada server usas `host`, `port` y, más adelante, su índice para elegir su `ServerConfig`. 8080 y 8081 son dos elementos de ese vector y acabarán siendo dos fds.

## 3. Listeners

`Server::configure` copia la config y llama a `_setupListeners`. Por cada server crea un `Socket`, ejecuta `listenOn` y guarda `fd → índice` en `_listenerConfigs`.

```63:72:src/Server.cpp
for (std::size_t i = 0; i < _configs.size(); ++i)
{
    listener = new Socket();
    if (!listener->listenOn(_configs[i].host, _configs[i].port))
        throw 0;
    _listenerConfigs[listener->fd()] = i;
    _listeners.push_back(listener);
}
```

`Socket::listenOn` aplica esta cadena:

```31:57:src/Socket.cpp
::getaddrinfo(...)
::socket(...)
::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, ...)
::bind(...)
::listen(fd, 128)
setNonBlocking(fd)
```

Conocimiento concreto:

- `getaddrinfo` traduce `127.0.0.1` y el puerto textual a una `sockaddr`. `freeaddrinfo` libera esa lista.
- `SO_REUSEADDR` permite reutilizar el puerto tras reiniciar.
- `bind` reserva la dirección; `listen(128)` crea la cola de conexiones pendientes.
- `AI_PASSIVE` y `AF_INET` fijan un listener TCP IPv4.
- `fcntl(fd, F_SETFL, O_NONBLOCK)` activa no bloqueante. No se usa `F_GETFL`: la whitelist de macOS solo autoriza `F_SETFL`, `O_NONBLOCK` y `FD_CLOEXEC`.
- Si falla un listener, se destruyen los anteriores. No queda un puerto abierto y otro no.

Pregunta probable: “¿por qué dos puertos no significan dos procesos?”. Porque ambos fds viven en `_listeners` y entran al mismo `poll`.

## 4. El único poll

Cada vuelta reconstruye el array y espera como máximo 500 ms.

```111:159:src/Server.cpp
entry.events = POLLIN;          // listener, si hay cupo
entry.events |= POLLIN;         // cliente leyendo request
entry.events |= POLLOUT;        // cliente con respuesta pendiente
entry.events = POLLOUT;         // pipe hacia stdin del CGI
entry.events = POLLIN;          // pipe desde stdout del CGI
ready = ::poll(&pollFds[0], pollFds.size(), 500);
```

Después solo se atiende un fd con `revents != 0`. El orden es: listener, pipe CGI, error del cliente, `POLLIN`, `POLLOUT` y `POLLHUP`. Al terminar la pasada se hace reap CGI y barrido de timeouts.

Límites definidos en [src/Server.cpp](../src/Server.cpp):

- 1024 clientes.
- 16 MiB de buffer de lectura por conexión.
- 1 MiB de respuesta aún no enviada. Al llegar ahí se retira `POLLIN`: es backpressure, para no seguir leyendo si el cliente no consume.
- Pausa de accept de 1 s si `accept` falla.
- `poll` devuelve también por timeout. Timeout no es error: permite revisar CGI y cierres aunque nadie hable.

Señales:

```15:19:src/Server.cpp
volatile sig_atomic_t g_stopRequested = 0;
void requestStop(int) { g_stopRequested = 1; }
```

`SIGINT` y `SIGTERM` solo escriben ese entero. `sig_atomic_t` es el tipo seguro dentro de un handler. La destrucción de conexiones ocurre al salir del bucle. `SIGPIPE` se ignora: escribir a un cliente desaparecido devuelve error, pero no mata el proceso.

`errno == EINTR` se consulta tras `poll` o `waitpid`, nunca tras `recv`, `send`, `read` o `write`. Si una de estas devuelve error o cero inesperado, la conexión se cierra o el CGI pasa a fallo. No hay bucle “si `EAGAIN`, reintenta”.

## 5. Accept y Connection

Cuando el listener tiene `POLLIN`, `_acceptNew` llama a `acceptClient`.

```77:96:src/Socket.cpp
clientFd = ::accept(...)
// sockaddr IPv4 -> "a.b.c.d" con ntohl y desplazamientos
setNonBlocking(clientFd)
```

No se usa `inet_ntop`: no está en la whitelist. `ntohl` sí está. La IP queda guardada para que kmarrero añada `REMOTE_ADDR`.

`Connection` es el estado de un cliente:

- `CONN_READING_REQUEST`: todavía llega la petición.
- `CONN_PROCESSING`: hay un CGI en marcha.
- `CONN_WRITING_RESPONSE`: `writeBuf` contiene la respuesta.
- `CONN_CLOSING`: el peer cerró o hay que terminar, pero quizá queda salida o un hijo CGI.

También guarda fd, buffers, `bytesSent`, request incremental, índice del server, IP, actividad y el puntero CGI. El mapa de `Server` usa el fd como clave. Por eso, mientras haya que identificar la conexión, no se hace `close` todavía: `shutdown(SHUT_RDWR)` corta el tráfico y el fd sigue siendo válido como clave.

## 6. Recepción — tu I/O, parser de kmarrero

```284:304:src/Server.cpp
count = ::recv(fd, buffer, 4096, 0);
readBuf.append(...)
request.parse(readBuf, parser);
readBuf.clear();
```

`recv > 0` aporta progreso y renueva el timeout. El parser puede quedar a medias: entonces retornas y el próximo `POLLIN` continúa. `Request` conserva lo ya interpretado; por eso el buffer temporal se vacía.

Tu responsabilidad termina en tres decisiones:

- `REQ_ERROR`: respondes 400, o el código indicado si ya es 4xx/413. No llamas al router.
- request completa: pides `router.match`. Sin location, 404.
- si `prepareCgi` devuelve true, arrancas el proceso. Si no, `handler.handle` crea la respuesta estática.

Esas clases son de kmarrero. Tú debes explicar el momento exacto en que las llamas y que una petición incompleta nunca entra ahí.

`recv == 0` significa cierre del peer. Si aún hay respuesta pendiente, pasas a `CONN_CLOSING`; si no, cierras.

## 7 y 8. Router, handler y response — frontera

No defiendes la gramática ni el mapeo de archivos. Sí defiendes el encaje:

1. La respuesta completa llega como `response.raw()`.
2. Sustituye `writeBuf`, pone el offset a cero y cambia el estado a escritura.
3. El disco lo lee el handler. El subject exime a los archivos regulares de `poll`; tu obligación empieza cuando esos bytes ya están en memoria y deben salir por el socket.

## 9. CGI — tu mitad

kmarrero decide si la extensión corresponde y construye el entorno. `prepareCgi` añade `REMOTE_ADDR` con la IP que guardaste. Aunque `buildEnv` esté en [src/CgiProcess.cpp](../src/CgiProcess.cpp), su significado HTTP es de él. Tú explicas qué haces con ese vector.

Antes de `fork`, recopilas todos los fds que el hijo no debe heredar: listeners, clientes y pipes CGI existentes.

```160:226:src/CgiProcess.cpp
pipe(inputPipe);   // padre escribe, hijo lee por stdin
pipe(outputPipe);  // hijo escribe por stdout, padre lee
fcntl(extremos del padre, F_SETFL, O_NONBLOCK);
pid = fork();
```

En el hijo:

- cierra los extremos que le pertenecen al padre;
- cierra todos los fds heredados;
- `dup2` conecta el pipe de entrada a stdin y el de salida a stdout;
- `chdir` al directorio del script, para que sus rutas relativas funcionen;
- `execve(python3, script, entorno)`;
- si `execve` vuelve, `exit(127)`.

En el padre cierras los extremos del hijo. Si no hay body, cierras también stdin inmediatamente: ese `close` entrega el EOF que el CGI espera. El body ya está deschunked; tú no interpretas chunks.

Durante el servicio los pipes usan el mismo `poll`:

```244:265:src/CgiProcess.cpp
count = ::write(_toChild, body + offset, restante);
// al completar: close(stdin) para producir EOF
```

```268:300:src/CgiProcess.cpp
count = ::read(_fromChild, buffer, 4096);
// count == 0: EOF, se cierra stdout
```

`tryReap` usa `waitpid(pid, &status, WNOHANG)`. Cero significa “sigue vivo”; no esperas. Solo un exit code 0 lleva a `CGI_DONE`. Entonces kmarrero convierte `output()` en una `Response`.

Si el pipe falla, `fail()` cierra pipes y marca `CGI_FAILED`, sin esperar al hijo. `terminate()` manda `SIGKILL` y deja el estado `CGI_TERMINATING`; el reap sigue siendo no bloqueante. El `waitpid` bloqueante existe solo en el destructor, es decir, durante la destrucción o el apagado. Su bucle únicamente ignora `EINTR`. Así no quedan zombies y el bucle normal nunca se bloquea.

El límite de salida CGI es 16 MiB. Superarlo falla el CGI en vez de crecer sin control.

## 10. Envío, timeouts y cierre

```391:407:src/Server.cpp
count = ::send(fd, writeBuf.c_str() + bytesSent, restante, 0);
addBytesSent(count);
```

`send` puede enviar solo una parte. `bytesSent` conserva el resto para la próxima vez que haya `POLLOUT`. Al terminar se limpia el buffer y se cierra la conexión. No hay keep-alive ni pipelining: una request, una response, cierre.

Timeouts en `_checkTimeouts`:

- 60 s sin progreso de I/O;
- 120 s para completar headers o para todo el estado CGI.

Mientras llega el body, sus bytes renuevan la actividad. Por eso un upload lento pero activo no muere a los 120 s; una petición que nunca termina los headers sí.

`_closeConnection` distingue dos cierres. Sin hijo activo, `delete` provoca el destructor de `Connection`, que borra el CGI y cierra el fd. Con hijo activo, hace `shutdown`, `SIGKILL`, deja `CONN_CLOSING` y conserva la entrada del mapa hasta que `waitpid` lo recoja. Después sí elimina la conexión.

`POLLHUP` con datos pendientes tampoco tira esos datos: marca cierre y deja que `POLLOUT` termine de enviarlos.

Memoria: los `new` de listeners, conexiones y CGI tienen un `delete` en destructor o limpieza. Los fallos de asignación caen en `catch (...)`: se cierra lo creado y el servidor sigue o sale de forma controlada, sin propagar la excepción.

## Preguntas que te pueden hacer

- ¿Dónde está el único `poll`? En `Server::run`, línea 159.
- ¿Qué fds vigila? Listeners, sockets de clientes y los dos extremos padre de cada CGI.
- ¿Por qué no lees el disco dentro de `poll`? Porque el subject solo exige readiness para sockets, pipes y FIFOs.
- ¿Qué pasa con un `send` corto? Avanza `bytesSent`; no descarta ni reenvía el principio.
- ¿Y si `recv` devuelve cero? El peer cerró. Se conserva la respuesta pendiente o se cierra.
- ¿Por qué cerrar stdin del CGI? Sin ese EOF, un CGI que lee hasta el final esperaría indefinidamente.
- ¿Por qué cerrar los fds heredados? Si el hijo conserva el listener, el puerto puede permanecer ocupado aunque el padre cierre.
- ¿Dónde se evita un zombie? `WNOHANG` en cada vuelta y `waitpid` bloqueante solo en el destructor.
- ¿Puede una request colgar? No: hay timeout de inactividad, timeout de fase, límite de buffer y error ante un peer que desaparece.
- ¿Qué no debes atribuirte? Lexer, directivas, locations, status HTTP, páginas de error y parsing de la salida CGI.
