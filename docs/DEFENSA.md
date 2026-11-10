# Defensa — recorrido de una request

Los dos explicáis los dos planos. El nombre dice quién lo escribió, no quién puede callarse.

Subject v24.1. Un proceso, C++98, un solo `poll`. `recv` / `send` / `read` / `write` de socket o pipe solo si ese fd salió listo. El disco no pasa por `poll`. `fork` solo para CGI. Python es el script (`www/cgi-bin/hello.py`) o un tester, no el servidor.


| Quién        | Código                                                                                                                                                        |
| ------------ | ------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **kebris-c** | `Socket`, `Server`, `Connection`, proceso CGI (`start`, pipes, `fork` / `execve` / `waitpid`), señales                                                        |
| **kmarrero** | config (`Lexer`, `Parser`, `StateMachine`, `Config`), `Request`*, `Router`, `HttpHandler`, `Response`, `CgiUtils`, `CgiProcess::buildEnv`, `configs/`, `www/` |
| **los dos**  | `main.cpp`, `Makefile`, `Utils`                                                                                                                               |


Arranque real: `./webserv configs/default.conf`. Sin ese argumento el programa sale. El subject acepta argumento **o** un path por defecto; aquí solo existe el argumento. `WEBSERV_DEFAULT_CONF` no se usa.

---



## Qué decir si os paran

- `root` quita el prefijo de la location. `/kapouet` + root `/tmp/www` + `/kapouet/pouic` → `/tmp/www/pouic`. En el sample, `/files` tiene root `www/files`.
- `GET /old` es 301 `Location: /` antes de mirar el disco.
- `error_page` se lee una vez desde `www/errors/`. Si el fichero no está, HTML mínimo. No hay segunda página de error.
- Un directorio sirve `location.index`. Si no está y `autoindex` está off, 403.
- El POST escribe solo en `upload_store`, un nombre, sin `/` ni `..`. DELETE borra el path mapeado.
- `client_max_body_size` del server vale para todas las rutas; el de la location solo para esa. `0` en la location hereda. Por encima, 413, sin CGI.
- El body chunked se deschunkéa antes del CGI. El script ve el body y EOF.
- Los headers del cliente van al CGI como `HTTP_*`, en minúsculas al guardar. `content-type` y `content-length` tienen su variable. `connection` y `transfer-encoding` no se copian.
- Disco con `stat`. El path del CGI puede ser relativo: `start` hace `chdir` y `execve`.
- Sin location, o con `..`, la respuesta es 404. `REQ_ERROR` es 400 o 413 y no entra al handler.

## Lo que sigue sin ser la whitelist

`unlink` y `rmdir` no están en la lista, y DELETE es obligatorio. No hay otra llamada de la lista que borre un fichero. Frase: «DELETE lo exige el subject; la lista no tiene primitiva de borrado; estas dos son el mínimo».

El `readBuf` corta a 16 MiB y cierra el fd sin status. Es el tope de abuso, por encima del 413 del config. No lo subáis.

Sin argumento el programa sale. El subject acepta argumento o un path por defecto; aquí solo está el argumento. `WEBSERV_DEFAULT_CONF` no se usa.

