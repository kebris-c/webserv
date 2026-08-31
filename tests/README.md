# Tests

OWNER: **kmarrero** (primary). **kebris-c** adds / requests stress and disconnect cases against the loop.

Subject allows external tests in Python (or Go, etc.). These scripts are **clients** — they are not part of the C++ server binary.

Deep guidance: [`../KMARRERO.md`](../KMARRERO.md) §10 · compliance: [`../SUBJECT_RULES.md`](../SUBJECT_RULES.md)

## Goal

Do not demo with “one lucky browser click” only. Automate regressions while kebris-c changes the poll loop.

## Implement `test_smoke.py`

```bash
# once the server answers HTTP:
python3 tests/test_smoke.py http://127.0.0.1:8080
```

Minimum cases (assert status + critical headers/body markers):

1. `GET /` → 200  
2. `GET /missing` → 404 + error page content  
3. `GET /files/` → autoindex when enabled  
4. Redirect `/old` → 301/302 + `Location`  
5. Forbidden method → 405  
6. `POST /upload` small body → stored file exists  
7. Oversized body → 413  
8. `DELETE` allowed vs denied  
9. CGI GET (+ POST when ready)  
10. Second port `8081` serves site2  
11. Malformed request → 400  
12. Chunked POST (after parser supports it)  

## Stress / hang helpers (pair with kebris-c)

- Parallel GETs (shell loop or Python thread pool) — server must stay up  
- Slow header drip over a raw socket (1 byte / 100ms) — must timeout, not freeze others  
- Disconnect mid-body  

## nginx compare

For any disputed behaviour, run the same `curl -v` against nginx with an equivalent config and keep both outputs in notes for defense.
