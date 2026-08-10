# Tests

OWNER: **kmarrero** (primary) — kebris-c adds stress / disconnect cases against the loop.

## Goal

Subject asks for resilience testing beyond a single browser click. Prefer Python scripts (fits scraper/web-tooling skills).

## Suggested cases (implement later)

1. `GET /` → 200 + body
2. `GET /missing` → 404 + error page
3. `POST /upload` with file → stored under `www/uploads`
4. Oversized body → 413
5. `DELETE` allowed vs denied → 204/200 vs 405
6. Redirect `/old` → 301/302 + `Location`
7. CGI `GET/POST` `/cgi-bin/hello.py`
8. Second port `8081` serves site2
9. Slow client / partial request / early disconnect (kebris-c)
10. Parallel connections stress (kebris-c)

## Compare with nginx

Run the same requests against nginx with an equivalent config and diff status lines / critical headers.

## Placeholder

```bash
# TODO(kmarrero): python3 tests/test_smoke.py http://127.0.0.1:8080
```
