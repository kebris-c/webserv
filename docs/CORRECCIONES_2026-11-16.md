**Correcciones de la evaluación HTTP — 16 de noviembre de 2026**

Se corrigieron los siete hallazgos distintos del informe inicial. El informe anterior conserva los resultados del binario previo; esta nota describe el código posterior a las correcciones.

**Chunked incremental.** `RequestContext` guarda ahora una fase específica: `CHUNK_SIZE`, `CHUNK_DATA` o `CHUNK_TRAILERS`. Al esperar más datos mantiene esa fase y el cursor. Un fragmento de contenido deja de interpretarse como una nueva línea de tamaño. El constructor y `reset()` inicializan el estado. Se probaron tamaños, cuerpos, delimitadores, extensiones y trailers divididos, incluso a un byte por envío; también se conservaron los errores 400 y 413 para entrada inválida o excesiva.

**Timeout CGI.** El barrido de timeouts prepara un 504, termina el CGI y deja la respuesta pendiente para `POLLOUT`. El hijo sigue recogiéndose mediante el mecanismo existente; no se añade otro `poll` ni escritura directa. Con el timeout real de 60 segundos, la prueba recibió 504 tras aproximadamente 64,736 segundos. Después no quedó ningún hijo CGI y el servidor siguió respondiendo. El cierre de conexiones HTTP incompletas conserva su comportamiento previo.

**Métodos.** Se valida primero que el método sea un token HTTP. Una sintaxis inválida sigue devolviendo 400; un método válido no implementado, como UNKNOWN o X-CUSTOM, devuelve 501. Los métodos implementados que una ruta no admite devuelven 405.

**Allow.** Los 405 anuncian los métodos efectivos permitidos por la location. Para una ruta GET, la respuesta incluye `Allow: GET, HEAD`.

**HEAD.** Se admite junto con GET, también en CGI. `Response::raw(headOnly)` conserva estado y cabeceras, incluido Content-Length, y omite el cuerpo. Esto se aplica también a errores. Se puede configurar HEAD explícitamente; permitir GET permite también HEAD.

**Autoindex anidado.** `handleGet` recibe la URI concreta de la petición y la pasa al autoindex. Los enlaces conservan el subdirectorio completo, se codifican los componentes de URL y se escapa el título HTML.

**URLs codificadas.** La ruta se decodifica una sola vez antes del routing, mientras que la query se conserva sin modificar. Espacios, signos reservados y porcentajes literales pueden recuperarse mediante sus enlaces. Las secuencias incompletas/incorrectas y los bytes de control se rechazan con 400. Las secuencias de traversal decodificadas siguen pasando por la comprobación del router y no permiten salir de la ruta.

**Verificación realizada**

- Compilación C++98 con `-Wall -Wextra -Werror`: correcta.
- Smoke test original: 19 comprobaciones superadas en copia temporal.
- Evaluación principal repetida: 56 de 56 comprobaciones superadas, sin fallos. Esta batería incluye el timeout real, navegación en Chromium, conflictos de puertos, errores CGI y una nueva ejecución de Valgrind.
- Nuevo `tests/test_regressions.py`: siete métodos de prueba superados, con múltiples subcasos; una prueba lenta queda desactivada por defecto. El mismo caso de timeout real sí se ejecutó en la evaluación principal y devolvió 504.
- Siege: tres rondas de 100 peticiones, cinco clientes, HTTP/1.1, 100 % de disponibilidad y 300 transacciones correctas. No se reinició el servidor entre rondas. Fue una prueba corta, no una garantía de funcionamiento indefinido.
- Valgrind: 1.768 allocs y 1.768 frees; cero bytes pendientes al salir y cero errores en el recorrido probado.
- Tras el timeout quedaron los tres listeners y ningún socket cliente ni hijo CGI.

Los 56 controles de la batería principal no son una nota de evaluación. El informe inicial tenía 59 porque incluía tres comprobaciones complementarias; los casos adicionales de fragmentación y codificación se ampliaron ahora en la batería de regresión.

Comandos para repetir las pruebas desde el repositorio:

```bash
make -j2
python3 tests/test_regressions.py
# Activar también la prueba lenta del timeout de 60 segundos:
WEBSERV_TEST_CGI_TIMEOUT=1 python3 tests/test_regressions.py
# Ejecutar el smoke test cuando 8080 y 8081 estén libres:
python3 tests/test_smoke.py
```

El smoke test crea y elimina sus ficheros de prueba bajo `www`; la nueva batería de regresión utiliza un directorio temporal y un puerto disponible.

Evidencias posteriores: [resultados](evaluation-2026-11-16/after-fixes/results.json), [respuestas HTTP](evaluation-2026-11-16/after-fixes/responses.json), [timeout CGI](evaluation-2026-11-16/after-fixes/cgi-timeout.json), [Siege](evaluation-2026-11-16/after-fixes/siege.json), [Valgrind](evaluation-2026-11-16/after-fixes/valgrind.txt).

SHA-256 del ejecutable verificado: `c2dd8087a2c05e08a8088fa9a0ba4c0d53a0ae9dcefd99d6e92cc5f28e3e6400`.
