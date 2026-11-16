**Actualización:** los hallazgos de esta evaluación inicial se corrigieron y verificaron después. Consulta [las correcciones y resultados posteriores](CORRECCIONES_2026-11-16.md). Los datos siguientes corresponden al binario anterior.

**Evaluación de webserv — 16 de noviembre de 2026**

Se ejecutó la lista facilitada por el usuario sobre el código actual del repositorio. No se modificó el código del servidor. Se utilizó una copia de `www`, configuraciones aisladas y clientes locales; los procesos iniciados para la evaluación se cerraron al terminar.

**Resultado observado:** las 19 comprobaciones del smoke test existente pasaron. La batería ampliada contiene 59 comprobaciones: 51 PASS y 8 FAIL. Dos FAIL corresponden a distintas reproducciones del mismo defecto chunked, por lo que hay siete hallazgos distintos. HEAD es una comprobación adicional de semántica HTTP, fuera del mínimo GET/POST/DELETE del subject. Estas cifras cuentan comprobaciones y no equivalen a una nota de evaluación.

Binario evaluado: SHA-256 `f3efbaf29957a2b835f90aab0499b4a4a00c921249143cd11a8d6f166fbc7e94`.

Herramientas: compilador C++98, curl, clientes Python HTTP/TCP, Siege 4.0.7, Chromium 159.0.8154.6 mediante Playwright y Valgrind 3.22.0. Siege y Chromium se prepararon en `/tmp`; no se instalaron globalmente. No había telnet: sus peticiones manuales se reprodujeron mediante sockets TCP, enviando los bytes HTTP explícitamente.

**1. Fundamentos, poll y reglas que pueden detener la evaluación**

Un servidor HTTP escucha conexiones TCP, recibe bytes, reconstruye una petición, decide qué recurso o programa corresponde a su URI y devuelve una respuesta con código, cabeceras y, cuando procede, cuerpo. TCP entrega un flujo: una llamada a `recv` puede contener media cabecera, una petición completa o más datos. Por eso el parser debe conservar estado entre llamadas.

La pregunta sobre el mecanismo de eventos pide explicar cómo se atienden muchos clientes sin bloquearse esperando a uno. Aquí se utiliza `poll`: se prepara una lista de descriptores e intereses, se espera actividad y se consulta `revents`. `POLLIN` permite atender lectura o aceptar una conexión; `POLLOUT` permite intentar escribir. La disponibilidad no garantiza que se transfieran todos los bytes solicitados.

El recorrido auditado es:

```text
Server::run -> poll
  listener + POLLIN -> _acceptNew -> Socket::acceptClient -> accept
  cliente + POLLIN  -> _onReadable -> recv -> parser -> router/handler
  cliente + POLLOUT -> _onWritable -> send -> avanzar bytesSent
  pipe CGI + POLLOUT -> _onCgiEvent -> onPipeWritable -> write
  pipe CGI + POLLIN/POLLHUP -> _onCgiEvent -> onPipeReadable -> read/EOF
```

Se encontró una única llamada operativa a `poll`, en `src/Server.cpp:159`, dentro del bucle principal. El conjunto incluye listeners, clientes y ambos extremos de los pipes CGI que maneja el padre. La preparación de intereses está en `src/Server.cpp:109` y siguientes; el despacho, en las líneas 167–213. No es obligatorio solicitar ambos eventos en cada socket todo el tiempo: debe ser el mismo mecanismo el que gestione lectura y escritura según el estado de cada conexión.

| Operación revisada | Código | Manejo observado |
|---|---|---|
| aceptar cliente | `src/Socket.cpp:77` | Solo se llama tras `POLLIN`; descriptor nuevo en modo no bloqueante. |
| recibir cliente | `src/Server.cpp:284` | Positivo: procesa esos bytes; cero: cierre o cierre tras escritura pendiente; negativo: elimina/cierra conexión. |
| enviar cliente | `src/Server.cpp:392` | Cero o negativo: cierra; positivo: avanza el contador de bytes, conservando lo pendiente. |
| escribir stdin CGI | `src/CgiProcess.cpp:251` | Cero o negativo: CGI fallido; positivo: avanza posición; al completar cierra stdin para dar EOF. |
| leer stdout CGI | `src/CgiProcess.cpp:276` | Positivo: acumula; cero: EOF; negativo: fallo CGI. |

No se encontró control basado en `errno` para los resultados de estas lecturas/escrituras. Las consultas a `errno` tras `poll`, `waitpid`, `stat` o la conversión `strtoul` son casos diferentes: no son el error de `recv/read/send/write` que prohíbe el texto entregado.

La regla de expulsar al cliente ante un error se cumple en el código de sockets. En los pipes CGI se registra el fallo del proceso y se genera una respuesta de error al cliente. No se inyectaron artificialmente todos los valores posibles de retorno de las llamadas del sistema: se revisaron esas ramas y se probaron 15 cierres con RST, peticiones fragmentadas, EOF de CGI y errores CGI. Los 15 RST no dejaron crecimiento de descriptores.

Los ficheros ordinarios están exentos de pasar por `poll`, pero pueden ocupar el hilo mientras se leen. `Response::setBodyFromFile` carga el fichero completo mediante un `ifstream` síncrono. Una descarga de 1 MiB simultánea con ocho GET pequeños terminó correctamente; sus latencias fueron 56,221; 0,623; 0,314; 0,282; 0,273; 0,293; 0,291 y 0,277 ms. Esto demuestra progreso en esta prueba, no ausencia absoluta de pausas con discos lentos o archivos arbitrariamente grandes.

Compilación: `make -j2` terminó correctamente con `-Wall -Wextra -Werror -std=c++98`. Una segunda ejecución devolvió “Nothing to be done”; el hash y la fecha de modificación del ejecutable quedaron idénticos. No se detectó reenlazado innecesario.

La revisión no encontró un incumplimiento evidente de las reglas de `poll`/I/O que obligan a parar inmediatamente. Esto no aprueba por sí solo el comportamiento HTTP que se comprueba después.

**2. Configuration: qué se debe poder cambiar y qué se probó**

Este apartado comprueba que las directivas tengan efecto real. Un parser que reconoce y almacena palabras no basta: cambiar una directiva debe cambiar el resultado observable.

| Capacidad | Prueba ejecutada | Resultado |
|---|---|---|
| varios puertos | Sitio principal en `127.0.0.1:55267`, segundo sitio en `127.0.0.1:57279` | 200 y cuerpos distintos. |
| varias interfaces | Segundo sitio en `127.0.0.2:55267`, compartiendo número de puerto con la primera interfaz | 200 y contenido del sitio correcto, también desde Chromium. |
| página 404 configurada | Se configuró `/errors/404.html` con `CUSTOM_404_A` | 404 con ese contenido. |
| modificar página 404 | Se cambió el fichero a `CUSTOM_404_B` sin reiniciar | Las peticiones posteriores devolvieron B. |
| error por defecto | Directorio sin index y sin autoindex; sin página 403 personalizada | 403 con HTML de respaldo. |
| límite por location | `/uploads`: `1K`; cuerpos de 1.024 y 1.025 bytes | 201 y 413 respectivamente. |
| heredar límite del server | Server: `4K`; `/inherit` sin límite propio; cuerpos de 2.048 y 4.097 bytes | 201 y 413. |
| rutas a directorios diferentes | `/files` apunta a `www/files` | Se recuperó el fichero exacto. |
| index configurable | `index home.html` en lugar de `index.html` | GET del directorio devolvió `INDEX_CUSTOM`. |
| métodos por ruta | DELETE sobre un fichero existente en ruta de solo GET | 405 y fichero intacto. |
| destino de subida configurable | URI `/uploads/binary.bin`, root `www/uploads`, upload_store `www/storage` | El fichero se creó en `www/storage`, no en root. |
| segundo destino de subida | `upload_store www/storage2/` | Se respetó el segundo directorio y la barra final. |

La configuración exacta está en [tested.conf](evaluation-2026-11-16/tested.conf). Los puertos son de esta ejecución y los servidores de prueba ya están apagados.

Se contrastaron los códigos con el [registro IANA](https://www.iana.org/assignments/http-status-codes/) y la [RFC 9110](https://www.rfc-editor.org/rfc/rfc9110.html). Para estos casos: 200 lectura o actualización correcta; 201 creación; 204 eliminación sin cuerpo; 301 redirección permanente; 400 petición malformada; 403 acceso prohibido; 404 recurso inexistente; 405 método no permitido; 413 cuerpo demasiado grande; 500 fallo interno; 501 funcionalidad/método no implementado.

**Fallo:** la respuesta 405 no incluye `Allow`. La [RFC 9110, §15.5.6](https://www.rfc-editor.org/rfc/rfc9110.html#section-15.5.6) exige enumerar los métodos permitidos en esa cabecera. El código 405 y la conservación del fichero fueron correctos; falta completar la respuesta.

**3. Basic checks: GET, POST, DELETE y peticiones desconocidas**

GET debe recuperar el recurso y un código apropiado. POST debe ejecutar la acción implementada por el servidor; en este proyecto una subida ordinaria guarda el cuerpo bajo `upload_store`. DELETE debe eliminar el recurso cuando esté permitido.

Se subieron 512 bytes binarios, se compararon con los bytes del disco y se recuperaron por HTTP. La creación devolvió 201. Sobrescribirlos con un solo byte devolvió 200 y dejó exactamente ese byte: no quedaron restos del contenido anterior. DELETE devolvió 204 con cuerpo vacío y el GET posterior devolvió 404. También se ejecutó el POST textual de la guía mediante curl y se conservaron sus respuestas en los logs.

Una petición `UNKNOWN / HTTP/1.1` no mató el servidor; devolvió 400. La defensa de “no crashea” sí se cumple en esta prueba. La elección del código es cuestionable: el método es un token sintácticamente válido y no implementado; la [RFC 9110, §9.1](https://www.rfc-editor.org/rfc/rfc9110.html#section-9.1) recomienda 501 para este caso. `checkMethod` solo acepta GET/POST/DELETE y `parseRequestLine` convierte el resto en 400 (`src/RequestUtils.cpp:24`, `src/RequestParser.cpp:85`).

HEAD también devuelve 400 y un cuerpo HTML. Esta es una comprobación adicional de compatibilidad HTTP: HEAD es un método válido. Si se implementa, su respuesta debe carecer de cuerpo; si no se implementa, no debería confundirse con una línea de petición malformada. El mínimo exigido en el subject citado sigue siendo GET, POST y DELETE; este hallazgo no debe mezclarse con una ausencia de esos tres métodos.

Una petición sin Host devolvió 400 y una cabecera dividida entre varios envíos TCP terminó en 200. El funcionamiento incremental de las cabeceras no demuestra que todos los estados del cuerpo sean incrementales.

**4. CGI: proceso, directorio, entrada, salida y errores**

CGI significa que el servidor ejecuta otro programa para generar la respuesta. El método, la query y metadatos se transmiten mediante variables de entorno; el cuerpo deschunked llega por stdin. El programa emite cabeceras CGI, una línea vacía y el cuerpo por stdout. Un script que escribe `Status: 201 Created` debe producir el correspondiente estado HTTP.

Resultados observados:

- GET a `hello.py?evaluation=yes`: 200 y `QUERY=evaluation=yes`.
- POST con `BODY IS HERE`: 200 y cuerpo recibido completo en el CGI.
- Script que abre `sibling.txt` por ruta relativa: 200 y `RELATIVE_CGI_OK`. El directorio de ejecución fue adecuado para esta prueba.
- Script que consulta `HTTP_X_EVALUATION`, `CONTENT_TYPE` y `CONTENT_LENGTH`: recibió `present`, `plain/text` y `3`.
- Script que termina con exit 1: 500.
- Script con error de sintaxis: 500.
- Script con cabeceras CGI malformadas: 500.
- Script con `Status: 201 Created`: 201.
- Mientras otro CGI permanecía en un bucle infinito con `sleep`, un GET normal siguió respondiendo.

**Fallo importante: chunked solo funciona cuando los datos llegan agrupados de determinadas formas.** El mismo POST con `5\r\nhello\r\n0\r\n\r\n` completo devolvió 200 y `POST_BODY=hello`. Fragmentándolo así, devolvió 400 y cerró la conexión durante el envío:

```python
parts = [
    b"POST /cgi-bin/hello.py HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n",
    b"5\r",
    b"\nhe",
    b"llo\r\n",
    b"0\r\n",
    b"\r\n",
]
```

La causa que muestra el código está en `parseChunkedBody`: al leer el tamaño se adelanta `chunkOffSet`, pero si todavía no llega todo el contenido se devuelve REQ_WAIT. Al reanudar se vuelve a llamar a `chunkCalculations` desde el cursor adelantado, tratando datos del chunk como si fueran el próximo tamaño (`src/RequestParser.cpp:200`, `src/RequestUtils.cpp:124`). El test original pasa porque su cuerpo chunked se envía agrupado y no cubre esta fragmentación.

**Fallo importante: el CGI infinito acaba sin error HTTP.** La conexión se cerró tras 64,528 segundos y el cliente recibió `RemoteDisconnected: Remote end closed connection without response`. El proceso CGI fue eliminado y el servidor siguió disponible, pero no llegó ningún 500/504 que explicara el problema al cliente. El barrido de timeouts llama a `_closeConnection` en lugar de preparar una respuesta HTTP (`src/Server.cpp:543`). Un cliente con petición incompleta también se cerró a los 64,528 segundos. El valor configurado es 60 segundos; la medición real incluye granularidad del reloj, planificación y condiciones de la ejecución.

**5. Check with a browser: renderizado y navegación reales**

Este apartado pide comprobar una interacción de navegador, no limitarse a que curl imprima HTML. El navegador realiza peticiones adicionales para CSS y otros recursos, sigue redirecciones y muestra páginas de error.

Se ejecutó Chromium 159.0.8154.6 en modo headless mediante Playwright. No fue una revisión manual de DevTools: se registró el tráfico equivalente en un HAR y se guardaron capturas.

Pasaron: index configurable, página estática del proyecto, CSS, página 404 personalizada, listado de `/files/`, redirección 301 seguida hasta `/`, segundo puerto y segunda interfaz. Se registraron nueve respuestas HTTP.

Una petición real del navegador incluyó Host, User-Agent, Accept, Accept-Encoding y Connection. La respuesta estática incluyó `Content-Type: text/html` y `Content-Length: 67`. Las cabeceras completas se conservan en [browser.har](evaluation-2026-11-16/browser.har).

Se encontraron dos problemas de navegación al ampliar los casos:

- En `/files/sub/`, el enlace a `deep.txt` se genera como `/files/deep.txt`, perdiendo `/sub/`. `handleGet` pasa `location->path` al autoindex en vez de la URI concreta (`src/HttpHandler.cpp:205`).
- El autoindex codifica `a b.txt` como `a%20b.txt`, pero el GET de esa URL devuelve 404 porque la ruta no se decodifica antes de buscar el fichero. El listado anuncia una URL que el propio servidor no resuelve correctamente.

Capturas: [inicio](evaluation-2026-11-16/browser-home.png), [404](evaluation-2026-11-16/browser-404.png), [autoindex](evaluation-2026-11-16/browser-autoindex.png).

**6. Port issues: interfaz, puerto y dos procesos**

Una interfaz es la dirección local donde escucha el socket; el puerto identifica el servicio en esa dirección. Dos sitios pueden compartir número de puerto si se enlazan a direcciones diferentes. Dos sockets escuchando simultáneamente sobre la misma pareja exacta necesitan una política de compartición o virtual hosts coherente.

Se probaron direcciones de loopback `127.0.0.1` y `127.0.0.2`; ambas son locales. Esto comprueba direcciones distintas, no una segunda tarjeta física.

- Sitios distintos en puertos distintos: contenido correcto en HTTP y navegador.
- Sitios distintos en direcciones distintas con el mismo puerto: contenido correcto.
- Dos bloques server con la misma interfaz:puerto: el segundo bind falló y el programa terminó con código 1 y `Error configuring server`.
- Segundo proceso con una configuración que comparte sockets con el primero: salió con código 1 y el mismo mensaje. El primer proceso continuó sirviendo 200.
- Configuración sin `location /`: una URI sin coincidencia devolvió 404 y el proceso no murió.

El resultado es coherente con no implementar virtual hosts por Host en una única pareja interfaz:puerto. `SO_REUSEADDR` permite reutilización en determinadas condiciones de TCP; no convierte dos listeners incompatibles en virtual hosts. No se encontró `SO_REUSEPORT`.

**7. Siege, disponibilidad, memoria y conexiones colgadas**

Se acordó el perfil suave: cinco clientes concurrentes, veinte peticiones por cliente y tres rondas. Cada ronda son cien peticiones; el total válido es de trescientas, sobre un mismo proceso sin reiniciarlo entre rondas.

Comando efectivo, con HTTP/1.1 configurado explícitamente en `siegerc`:

```bash
siege -R siegerc -b -c 5 -r 20 --no-parser -j http://127.0.0.1:55267/empty.txt
```

`-b` elimina pausas deliberadas entre peticiones; `-c 5` limita la concurrencia; `-r 20` limita las repeticiones de cada cliente; `--no-parser` evita que recursos HTML secundarios alteren el número de solicitudes; `-j` conserva estadísticas JSON. La URL fue un fichero vacío que devolvió 200 y `Content-Length: 0`.

| Ronda | Transacciones | Correctas | Fallidas | Disponibilidad | RSS | Descriptores |
|---|---:|---:|---:|---:|---:|---:|
| Antes | — | — | — | — | 4.063.232 B | 6 |
| 1 | 100 | 100 | 0 | 100 % | 4.063.232 B | 6 |
| 2 | 100 | 100 | 0 | 100 % | 4.063.232 B | 6 |
| 3 | 100 | 100 | 0 | 100 % | 4.063.232 B | 6 |

Supera el umbral solicitado de 99,5 % en esta carga. Cada ronda duró aproximadamente 0,01 segundos según la resolución de Siege: es una prueba muy breve. Su cifra de 10.000 transacciones/s no es una medida sólida de capacidad sostenida y no se presenta como tal.

Inicialmente una configuración mínima de Siege hizo que utilizase su valor por defecto HTTP/1.0. El servidor devolvía 400, Siege mostraba disponibilidad del 100 % pero cero transacciones correctas. Esas rondas se descartaron como validación de GET correctos y se repitieron con HTTP/1.1 explícito. Los logs iniciales se conservan para no confundir disponibilidad de transporte con éxito HTTP.

Al finalizar quedaron seis descriptores: tres estándar y tres listeners. No quedaron conexiones cliente ni hijos CGI. La petición incompleta de la prueba de timeout se cerró y los 15 resets de clientes no dejaron crecimiento observado de descriptores.

Valgrind realizó un recorrido con GET, autoindex, 404, POST, recuperación, DELETE, CGI GET/POST y CGI fallido, y se cerró el servidor ordenadamente:

```text
in use at exit: 0 bytes in 0 blocks
1,765 allocs, 1,765 frees
All heap blocks were freed -- no leaks are possible
ERROR SUMMARY: 0 errors from 0 contexts
```

La frase de Valgrind se refiere a esa ejecución. No demuestra que sea imposible cualquier fuga en rutas no probadas. El descriptor extra abierto en la salida de Valgrind era su propio fichero de log, no un socket del servidor.

Tampoco se ejecutó Siege indefinidamente: se respetó el perfil acotado elegido. La memoria estable en tres rondas cortas y una ejecución limpia de Valgrind son evidencias útiles, pero no sustituyen una prueba de horas de duración.

**8. Prioridad de los hallazgos**

1. Corregir la conservación de estado del parser chunked: un transporte TCP fragmentado válido se rechaza.
2. Responder con un error HTTP antes de cerrar el cliente por timeout CGI, manteniendo la terminación/recolección del hijo.
3. Separar método no implementado de petición malformada; responder 501 al método desconocido válido y completar los 405 con Allow.
4. Generar autoindex con la URI real y resolver correctamente URLs codificadas, aplicando validación de rutas tras decodificar.
5. Revisar HEAD como compatibilidad adicional, distinguiéndolo del mínimo exigido GET/POST/DELETE.

No se han aplicado estas correcciones durante la evaluación, para que los resultados describan el código probado.

**9. Evidencias y reproducción**

[Resultados de las 59 comprobaciones](evaluation-2026-11-16/results.json), [respuestas HTTP registradas](evaluation-2026-11-16/responses.json), [estadísticas Siege](evaluation-2026-11-16/siege.json), [reproducción chunked](evaluation-2026-11-16/chunked-reproduction.json), [timeout CGI](evaluation-2026-11-16/cgi-timeout.json), [registro Valgrind](evaluation-2026-11-16/logs/valgrind-7232.txt).

Los scripts [evaluate.py](evaluation-2026-11-16/evaluate.py) y [followup.py](evaluation-2026-11-16/followup.py) están conservados para inspección. Usan rutas absolutas y herramientas preparadas en `/tmp/webserv-evaluation-2026-11-16`; si se quieren ejecutar en otra máquina hay que adaptar esas rutas y preparar herramientas. `followup.py` repite únicamente comprobaciones instrumentales corregidas y añade reproducciones; no hace falta volver a ejecutar todas las pruebas para leer el informe.

Los errores del propio script se corrigieron antes de producir los resultados finales: conflicto entre un nombre de función y el módulo HTTP, y uso de `net_connections` en una versión de psutil que expone `connections`. No se cuentan como fallos de webserv. Las rondas Siege inválidas por HTTP/1.0 tampoco se cuentan como aprobación de GET.

**10. Registro completo de comprobaciones ampliadas**

Dos filas describen el mismo defecto chunked en fases distintas. Se mantienen ambas porque una detectó el corte de conexión y la otra capturó la respuesta 400 que lo causaba.

| Nº | Comprobación | Resultado | Evidencia resumida |
|---:|---|---|---|
| 1 | Configured directory index | PASS | HTTP 200; 67 bytes |
| 2 | Different site on second port | PASS | HTTP 200; 41 bytes |
| 3 | Different site on different interface, same port | PASS | HTTP 200; 41 bytes |
| 4 | Route maps to a different directory | PASS | HTTP 200; 60 bytes |
| 5 | Configured 404 page A | PASS | HTTP 404; 21 bytes |
| 6 | Modified 404 page without restart | PASS | HTTP 404; 21 bytes |
| 7 | Default error page fallback | PASS | HTTP 403; 48 bytes |
| 8 | DELETE forbidden by route | PASS | HTTP 405; 199 bytes |
| 9 | Forbidden DELETE preserves file | PASS | Condición comprobada |
| 10 | 405 includes mandatory Allow header | FAIL | 405 response has no Allow header |
| 11 | GET directory autoindex | PASS | HTTP 200; 194 bytes |
| 12 | GET missing file | PASS | HTTP 404; 21 bytes |
| 13 | 301 redirect and Location | PASS | HTTP 301; 0 bytes |
| 14 | Binary POST creates uploaded file | PASS | HTTP 201; 12 bytes |
| 15 | upload_store differs from root | PASS | Condición comprobada |
| 16 | Retrieve binary upload | PASS | HTTP 200; 512 bytes |
| 17 | Overwrite uploaded file | PASS | HTTP 200; 13 bytes |
| 18 | Overwrite truncates correctly | PASS | HTTP 200; 1 bytes |
| 19 | Second upload_store with trailing slash | PASS | HTTP 201; 12 bytes |
| 20 | Second upload destination on disk | PASS | Condición comprobada |
| 21 | DELETE permitted | PASS | HTTP 204; 0 bytes |
| 22 | GET after DELETE | PASS | HTTP 404; 21 bytes |
| 23 | POST at exact location body limit | PASS | HTTP 201; 12 bytes |
| 24 | POST above location body limit | PASS | HTTP 413; 56 bytes |
| 25 | POST inherits server body limit | PASS | HTTP 201; 12 bytes |
| 26 | POST above server body limit | PASS | HTTP 413; 56 bytes |
| 27 | Unknown method does not crash | PASS | HTTP 400; 123 bytes |
| 28 | Unknown method uses 501 rather than 400 | FAIL | b'HTTP/1.1 400 Bad Request\r\nContent-Length: 50\r\nContent-Type: text/html\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>' |
| 29 | Valid HEAD returns allowed-method response | FAIL | b'HTTP/1.1 400 Bad Request\r\nContent-Length: 50\r\nContent-Type: text/html\r\n\r\n<html><body><h1>400 Bad Request</h1></body></html>' |
| 30 | Missing Host is rejected | PASS | HTTP 400; 123 bytes |
| 31 | Fragmented request | PASS | HTTP 200; 131 bytes |
| 32 | CGI GET with query | PASS | HTTP 200; 55 bytes |
| 33 | CGI POST | PASS | HTTP 200; 65 bytes |
| 34 | CGI relative working directory | PASS | HTTP 200; 16 bytes |
| 35 | CGI headers and environment | PASS | HTTP 200; 51 bytes |
| 36 | Incremental chunked CGI POST | FAIL | [Errno 32] Broken pipe |
| 37 | CGI nonzero exit | PASS | HTTP 500; 190 bytes |
| 38 | CGI syntax error | PASS | HTTP 500; 190 bytes |
| 39 | CGI invalid headers | PASS | HTTP 500; 190 bytes |
| 40 | CGI honors Status header | PASS | HTTP 201; 12 bytes |
| 41 | Server responds during infinite CGI | PASS | HTTP 200; 67 bytes |
| 42 | curl commands from the checklist | PASS | {"commands": 2, "get": 200, "post": 201} |
| 43 | Client resets release descriptors | PASS | {"resets": 15, "fds_before": 9, "fds_after": 9} |
| 44 | Duplicate interface:port in configuration | PASS | {"exit": 1, "stderr": "Error configuring server"} |
| 45 | Two processes sharing interface:port | PASS | {"exit": 1, "stderr": "Error configuring server", "first_server": "alive"} |
| 46 | Real Chromium navigation and network capture | PASS | {"version": "159.0.8154.6", "responses": 9, "screenshots": 3, "har": "/tmp/webserv-evaluation-2026-11-16/browser.har"} |
| 47 | Nested autoindex link reaches correct resource | FAIL | Generated link uses /files/deep.txt instead of /files/sub/deep.txt |
| 48 | Encoded filename from directory listing | FAIL | Expected 200, got 404: b'<h1>CUSTOM_404_B</h1>' |
| 49 | Idle partial request is closed (60-second configured timeout) | PASS | {"seconds": 64.528, "response": ""} |
| 50 | CGI infinite loop returns an HTTP error | FAIL | Infinite CGI did not return an HTTP error: {'exception': 'RemoteDisconnected', 'error': 'Remote end closed connection without response', 'seconds': 64.528} |
| 51 | Server still available after CGI timeout | PASS | HTTP 200; 67 bytes |
| 52 | No CGI children after timeout | PASS | Condición comprobada |
| 53 | No location /: unmatched URI is safe | PASS | HTTP 404; 48 bytes |
| 54 | Valgrind: requests, CGI and clean shutdown | PASS | 0 bytes al salir; 0 errores; 1.765 allocs/frees |
| 55 | Siege 3 rounds: -b -c 5 -r 20 | PASS | 3 × 100 correctas; disponibilidad 100 %; RSS y FD estables |
| 56 | No hanging client descriptors after completion | PASS | {"fds": 6, "sockets": [{"fd": 5, "status": "LISTEN"}, {"fd": 4, "status": "LISTEN"}, {"fd": 3, "status": "LISTEN"}], "children": 0} |
| 57 | Reproduce whole versus fragmented chunked CGI request | FAIL | Fragmented chunked request rejected: HTTP/1.1 400 Bad Request Content-Length: 50 Content-Type: text/html  <html><body><h1>400 Bad Request</h1></body></html> |
| 58 | Regular-file read and concurrent small GET progress | PASS | {"file_bytes": 1048576, "small_requests": 8, "latency_ms": [56.221, 0.623, 0.314, 0.282, 0.273, 0.293, 0.291, 0.277]} |
| 59 | GET empty file returns 200 with zero bytes | PASS | HTTP 200; 0 bytes |
