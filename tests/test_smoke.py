#!/usr/bin/env python3
"""OWNER: kmarrero — smoke tests placeholder. Implement once the server answers HTTP."""

# LEARN: http.client or requests; raw sockets for partial/slow clients.
# Do not mark the project done until browser + these scripts both pass.

import sys

def main() -> int:
    base = sys.argv[1] if len(sys.argv) > 1 else "http://127.0.0.1:8080"
    print("TODO: implement smoke tests against", base)
    print("Cases listed in tests/README.md")
    return 1

if __name__ == "__main__":
    raise SystemExit(main())
