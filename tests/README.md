# Tests

External clients used to exercise `webserv`. Subject allows tests in Python (or
similar). These scripts are **not** part of the C++ server binary.

## Smoke

```bash
python3 tests/test_smoke.py http://127.0.0.1:8080
```

Suggested checks (status + critical headers/body markers):

1. `GET /` → 200
2. `GET /missing` → 404 + error page content
3. `GET /files/` → autoindex when enabled
4. Redirect `/old` → 301/302 + `Location`
5. Forbidden method → 405
6. `POST /upload` small body → stored file exists
7. Oversized body → 413
8. `DELETE` allowed vs denied
9. CGI GET and POST
10. Second port `8081` serves site2
11. Malformed request → 400
12. Chunked POST

## Stress / hang helpers

- Parallel GETs — server must stay up
- Slow header drip over a raw socket — must timeout, not freeze others
- Disconnect mid-body

## nginx compare

For disputed behaviour, run the same `curl -v` against nginx with an equivalent
config and keep both outputs for defense.
