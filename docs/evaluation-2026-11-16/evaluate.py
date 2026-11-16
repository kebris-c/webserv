#!/usr/bin/env python3
"""Isolated execution of the supplied webserv evaluation checklist."""
import concurrent.futures
import contextlib
import hashlib
from http.client import HTTPConnection
import json
import os
from pathlib import Path
import re
import shutil
import signal
import socket
import struct
import subprocess
import sys
import time
import traceback

import psutil

BASE = Path('/tmp/webserv-evaluation-2026-11-16')
REPO = Path('/home/kjroydev/proyectos/webserv')
RUN = BASE / 'run'
LOG = BASE / 'logs'
if RUN.exists():
    shutil.rmtree(RUN)
RUN.mkdir(exist_ok=True)
LOG.mkdir(exist_ok=True)
shutil.copytree(REPO / 'www', RUN / 'www', dirs_exist_ok=True)
RESULTS = []
RESPONSES = []


def save():
    (BASE / 'results.json').write_text(json.dumps(RESULTS, indent=2, ensure_ascii=False))
    (BASE / 'responses.json').write_text(json.dumps(RESPONSES, indent=2, ensure_ascii=False))


def test(name, fn):
    start = time.monotonic()
    try:
        details = fn()
        result = dict(name=name, result='PASS', seconds=round(time.monotonic()-start, 3), details=details)
    except Exception as exc:
        result = dict(name=name, result='FAIL', seconds=round(time.monotonic()-start, 3),
                      error=str(exc), traceback=traceback.format_exc())
    RESULTS.append(result)
    save()
    print(result['result'], name, json.dumps(result.get('details', result.get('error')), ensure_ascii=False), flush=True)
    return result


def ensure(value, message):
    if not value:
        raise AssertionError(message)


def free_port(host='127.0.0.1'):
    with socket.socket() as sock:
        sock.bind((host, 0))
        return sock.getsockname()[1]


P1, P2 = free_port(), free_port()


def http(method='GET', path='/', body=None, headers=None, host='127.0.0.1', port=None, timeout=5):
    start = time.monotonic()
    conn = HTTPConnection(host, port or P1, timeout=timeout)
    try:
        conn.request(method, path, body=body, headers=headers or {})
        response = conn.getresponse()
        data = response.read()
        entry = dict(method=method, path=path, host=host, port=port or P1,
                     status=response.status, reason=response.reason,
                     headers=dict(response.getheaders()), bytes=len(data),
                     sha256=hashlib.sha256(data).hexdigest(),
                     sample=data[:300].decode(errors='replace'), seconds=round(time.monotonic()-start, 4))
        RESPONSES.append(entry)
        return response.status, dict((k.lower(),v) for k,v in response.getheaders()), data
    finally:
        conn.close()


def expect_http(code, *args, contains=None, exact=None, **kwargs):
    actual, headers, body = http(*args, **kwargs)
    ensure(actual == code, f'Expected {code}, got {actual}: {body[:120]!r}')
    if contains is not None:
        ensure(contains in body, f'Missing {contains!r} from {body[:200]!r}')
    if exact is not None:
        ensure(body == exact, f'Body mismatch: received {len(body)} bytes, expected {len(exact)}')
    if 'content-length' in headers:
        ensure(int(headers['content-length']) == len(body), 'Wrong Content-Length')
    return dict(status=actual, bytes=len(body), headers=headers)


def raw(parts, delay=.005, timeout=5):
    with socket.create_connection(('127.0.0.1', P1), timeout=timeout) as client:
        client.settimeout(timeout)
        for part in parts:
            client.sendall(part)
            if delay:
                time.sleep(delay)
        output = bytearray()
        while True:
            block = client.recv(65536)
            if not block:
                break
            output.extend(block)
    return bytes(output)


def expect_raw(code, parts, contains=None):
    response = raw(parts)
    ensure(response.startswith(f'HTTP/1.1 {code} '.encode()), repr(response[:200]))
    if contains:
        ensure(contains in response, repr(response[:300]))
    return dict(status=code, bytes=len(response), sample=response[:250].decode(errors='replace'))


@contextlib.contextmanager
def server(config, name, port=P1, prefix=None):
    path = RUN / (name + '.conf')
    path.write_text(config)
    with (LOG / (name + '.stdout')).open('w') as out, (LOG / (name + '.stderr')).open('w') as err:
        process = subprocess.Popen((prefix or []) + [str(REPO/'webserv'), str(path)], cwd=RUN, stdout=out, stderr=err)
        try:
            for _ in range(150):
                if process.poll() is not None:
                    raise RuntimeError(f'Server exited {process.returncode}: {(LOG/(name+".stderr")).read_text()}')
                try:
                    with socket.create_connection(('127.0.0.1', port), timeout=.1):
                        break
                except OSError:
                    time.sleep(.03)
            else:
                raise TimeoutError('No listening socket')
            yield process
        finally:
            if process.poll() is None:
                process.send_signal(signal.SIGINT)
                try:
                    process.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


for directory in ('storage', 'storage2', 'readonly', 'noindex'):
    (RUN/'www'/directory).mkdir(exist_ok=True)
(RUN/'www/home.html').write_text('<!doctype html><title>Configured index</title><h1>INDEX_CUSTOM</h1>')
(RUN/'www/empty.txt').write_bytes(b'')
(RUN/'www/readonly/keep.txt').write_text('DO_NOT_DELETE')
(RUN/'www/storage/conflict').mkdir(exist_ok=True)
(RUN/'www/files/sub').mkdir(exist_ok=True)
(RUN/'www/files/sub/deep.txt').write_text('NESTED_FILE')
(RUN/'www/files/a b.txt').write_text('SPACE_FILE')
(RUN/'www/site2/home.html').write_text('<title>Second</title><h1>SITE_SECOND</h1>')
(RUN/'www/errors/404.html').write_text('<h1>CUSTOM_404_A</h1>')

scripts = {
    'relative.py': "from pathlib import Path\nprint('Content-Type: text/plain\\n')\nprint(Path('sibling.txt').read_text())\n",
    'exit.py': 'import sys\nsys.exit(1)\n',
    'syntax.py': 'this is not valid python !!!\n',
    'loop.py': 'import time\nwhile True: time.sleep(.05)\n',
    'headers.py': "import os,sys\nb=sys.stdin.buffer.read()\nprint('Content-Type: text/plain\\n')\nprint('QUERY='+os.environ.get('QUERY_STRING',''))\nprint('HEADER='+os.environ.get('HTTP_X_EVALUATION',''))\nprint('CTYPE='+os.environ.get('CONTENT_TYPE',''))\nprint('LEN='+os.environ.get('CONTENT_LENGTH',''))\nsys.stdout.flush()\nsys.stdout.buffer.write(b)\n",
    'malformed.py': "print('not a CGI header\\n\\nbody')\n",
    'status.py': "print('Status: 201 Created\\nContent-Type: text/plain\\n\\nCREATED_CGI')\n",
}
for filename, source in scripts.items():
    (RUN/'www/cgi-bin'/filename).write_text(source)
(RUN/'www/cgi-bin/sibling.txt').write_text('RELATIVE_CGI_OK')

CONFIG = f'''server {{
 listen 127.0.0.1:{P1};
 server_name localhost;
 client_max_body_size 4K;
 error_page 404 /errors/404.html;
 error_page 405 /errors/405.html;
 error_page 500 /errors/500.html;
 location / {{ root www; index home.html; allowed_methods GET; autoindex off; }}
 location /files {{ root www/files; allowed_methods GET; autoindex on; }}
 location /uploads {{ root www/uploads; upload_store www/storage; allowed_methods POST GET DELETE; client_max_body_size 1K; }}
 location /ingest2 {{ root www/uploads; upload_store www/storage2/; allowed_methods POST; }}
 location /stored {{ root www/storage; allowed_methods GET DELETE; }}
 location /inherit {{ root www/uploads; upload_store www/storage; allowed_methods POST; }}
 location /readonly {{ root www/readonly; allowed_methods GET; }}
 location /noindex {{ root www/noindex; allowed_methods GET; autoindex off; }}
 location /old {{ return 301 /; }}
 location /cgi-bin {{ root www/cgi-bin; allowed_methods GET POST; cgi_extension .py; cgi_pass /usr/bin/python3; }}
}}
server {{ listen 127.0.0.1:{P2}; location / {{ root www/site2; index home.html; allowed_methods GET; }} }}
server {{ listen 127.0.0.2:{P1}; location / {{ root www/site2; index home.html; allowed_methods GET; }} }}
'''
(BASE/'endpoints.json').write_text(json.dumps(dict(main=P1, second=P2, other_interface='127.0.0.2')))


def timeout_cgi():
    start = time.monotonic()
    try:
        status, _, body = http(path='/cgi-bin/loop.py', timeout=68)
        outcome = dict(status=status, body=body[:100].decode(errors='replace'))
    except Exception as exc:
        outcome = dict(exception=type(exc).__name__, error=str(exc))
    outcome['seconds'] = round(time.monotonic()-start, 3)
    (BASE/'cgi-timeout.json').write_text(json.dumps(outcome, indent=2))
    ensure(outcome.get('status') in (500, 502, 504), f'Infinite CGI did not return an HTTP error: {outcome}')
    return outcome


def idle_timeout():
    start = time.monotonic()
    with socket.create_connection(('127.0.0.1', P1), timeout=3) as client:
        client.sendall(b'GET / HTTP/1.1\r\nHost:')
        client.settimeout(68)
        data = client.recv(4096)
    ensure(not data or data.startswith(b'HTTP/1.1 408'), repr(data))
    return dict(seconds=round(time.monotonic()-start, 3), response=data.decode(errors='replace'))


def browser_tests():
    from playwright.sync_api import sync_playwright
    with sync_playwright() as p:
        browser = p.chromium.launch(headless=True, args=['--no-sandbox', '--no-proxy-server'])
        context = browser.new_context(record_har_path=str(BASE/'browser.har'), viewport={'width':1100,'height':780})
        page = context.new_page()
        events = []
        page.on('response', lambda r: events.append(dict(url=r.url, status=r.status, headers=r.headers)))
        url = f'http://127.0.0.1:{P1}'
        response = page.goto(url+'/', wait_until='networkidle')
        ensure(response.status == 200 and 'INDEX_CUSTOM' in page.content(), 'Browser index')
        response = page.goto(url+'/index.html', wait_until='networkidle')
        ensure(response.status == 200 and page.locator('h1').inner_text() == 'webserv', 'Browser static site')
        ensure(any('/style.css' in x['url'] and x['status']==200 for x in events), 'CSS did not load')
        page.screenshot(path=str(BASE/'browser-home.png'), full_page=True)
        response = page.goto(url+'/missing', wait_until='networkidle')
        ensure(response.status == 404 and 'CUSTOM_404_B' in page.content(), 'Browser 404')
        page.screenshot(path=str(BASE/'browser-404.png'), full_page=True)
        response = page.goto(url+'/files/', wait_until='networkidle')
        ensure(response.status == 200 and page.locator('a').count() >= 2, 'Browser listing')
        page.screenshot(path=str(BASE/'browser-autoindex.png'), full_page=True)
        response = page.goto(url+'/old', wait_until='networkidle')
        ensure(response.status == 200 and page.url.endswith('/'), 'Browser redirect')
        ensure(any(x['status']==301 for x in events), 'No 301 observed')
        response = page.goto(f'http://127.0.0.1:{P2}/', wait_until='networkidle')
        ensure(response.status == 200 and 'SITE_SECOND' in page.content(), 'Browser second port')
        response = page.goto(f'http://127.0.0.2:{P1}/', wait_until='networkidle')
        ensure(response.status == 200 and 'SITE_SECOND' in page.content(), 'Browser second interface')
        version = browser.version
        (BASE/'browser-responses.json').write_text(json.dumps(events, indent=2))
        context.close()
        browser.close()
        return dict(version=version, responses=len(events), screenshots=3, har=str(BASE/'browser.har'))


def siege_tests(process):
    binary = BASE/'tools/siege/usr/bin/siege'
    rc = BASE/'siegerc'
    rc.write_text('verbose = false\ncolor = off\nlogging = false\nparser = false\nconnection = close\nprotocol = HTTP/1.1\n')
    snapshots = []
    proc = psutil.Process(process.pid)
    def snapshot(label):
        time.sleep(.15)
        snapshots.append(dict(label=label, rss=proc.memory_info().rss, fds=proc.num_fds(), children=len(proc.children())))
    snapshot('before')
    stats = []
    for round_ in range(1,4):
        command = [str(binary), '-R', str(rc), '-b', '-c', '5', '-r', '20', '--no-parser', '-j', f'http://127.0.0.1:{P1}/empty.txt']
        done = subprocess.run(command, cwd=RUN, capture_output=True, text=True, timeout=30)
        (LOG/f'siege-{round_}.stdout').write_text(done.stdout)
        (LOG/f'siege-{round_}.stderr').write_text(done.stderr)
        ensure(done.returncode == 0, f'Siege exit {done.returncode}: {done.stderr}')
        match = re.search(r'\{.*\}', done.stdout, re.S)
        ensure(match, f'No Siege JSON: {done.stdout} {done.stderr}')
        data = json.loads(match.group())
        stats.append(data)
        snapshot(f'round-{round_}')
        print('SIEGE', round_, data, flush=True)
    result = dict(command=command, rounds=stats, snapshots=snapshots)
    (BASE/'siege.json').write_text(json.dumps(result, indent=2))
    for data in stats:
        ensure(float(data['availability']) > 99.5, f'Availability {data}')
        ensure(int(data['successful_transactions']) == 100, f'Incomplete round: {data}')
        ensure(int(data['failed_transactions']) == 0, f'Failed transactions: {data}')
    return result


def duplicate_config():
    config = f'''server {{ listen 127.0.0.1:{free_port()}; location / {{ root www; index home.html; }} }}'''
    path = RUN/'duplicate.conf'
    path.write_text(config+'\n'+config+'\n')
    result = subprocess.run([str(REPO/'webserv'), str(path)], cwd=RUN, capture_output=True, text=True, timeout=3)
    ensure(result.returncode > 0, f'Expected startup error: {result.returncode}')
    return dict(exit=result.returncode, stderr=result.stderr.strip())


def conflict_process():
    result = subprocess.run([str(REPO/'webserv'), str(RUN/'main.conf')], cwd=RUN, capture_output=True, text=True, timeout=3)
    ensure(result.returncode > 0, f'Expected bind failure: {result.returncode}')
    expect_http(200, contains=b'INDEX_CUSTOM')
    return dict(exit=result.returncode, stderr=result.stderr.strip(), first_server='alive')


def reset_clients(process):
    proc = psutil.Process(process.pid)
    before = proc.num_fds()
    for _ in range(15):
        client = socket.create_connection(('127.0.0.1', P1), timeout=3)
        client.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack('ii',1,0))
        client.sendall(b'GET / HTTP/1.1\r\nHost: localhost\r\n\r\n')
        client.close()
    time.sleep(.4)
    expect_http(200)
    time.sleep(.2)
    after = proc.num_fds()
    ensure(after <= before+1, f'FD growth {before}->{after}')
    return dict(resets=15, fds_before=before, fds_after=after)


def curl_tests():
    command = ['curl', '--noproxy', '*', '-sS', '-i', f'http://127.0.0.1:{P1}/']
    done = subprocess.run(command, capture_output=True, timeout=5)
    (LOG/'curl-get.txt').write_bytes(done.stdout)
    ensure(done.returncode==0 and done.stdout.startswith(b'HTTP/1.1 200'), repr(done.stdout))
    command = ['curl', '--noproxy', '*', '-sS', '-i', '-X', 'POST', '-H', 'Content-Type: plain/text', '--data', 'BODY IS HERE', f'http://127.0.0.1:{P1}/uploads/curl.txt']
    done = subprocess.run(command, capture_output=True, timeout=5)
    (LOG/'curl-post.txt').write_bytes(done.stdout)
    ensure(done.returncode==0 and done.stdout.startswith(b'HTTP/1.1 201'), repr(done.stdout))
    return dict(commands=2, get=200, post=201)


with server(CONFIG, 'main') as process, concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:
    print('SERVER', process.pid, 'PORTS', P1, P2, flush=True)
    pending_cgi = pool.submit(test, 'CGI infinite loop returns an HTTP error', timeout_cgi)
    pending_idle = pool.submit(test, 'Idle partial request closes within 60 seconds', idle_timeout)
    test('Configured directory index', lambda: expect_http(200, contains=b'INDEX_CUSTOM'))
    test('Different site on second port', lambda: expect_http(200, port=P2, contains=b'SITE_SECOND'))
    test('Different site on different interface, same port', lambda: expect_http(200, host='127.0.0.2', contains=b'SITE_SECOND'))
    test('Route maps to a different directory', lambda: expect_http(200, path='/files/readme.txt', exact=(RUN/'www/files/readme.txt').read_bytes()))
    test('Configured 404 page A', lambda: expect_http(404, path='/absent', contains=b'CUSTOM_404_A'))
    (RUN/'www/errors/404.html').write_text('<h1>CUSTOM_404_B</h1>')
    test('Modified 404 page without restart', lambda: expect_http(404, path='/absent', contains=b'CUSTOM_404_B'))
    test('Default error page fallback', lambda: expect_http(403, path='/noindex/', contains=b'403 Forbidden'))
    test('DELETE forbidden by route', lambda: expect_http(405, 'DELETE', '/readonly/keep.txt'))
    test('Forbidden DELETE preserves file', lambda: ensure((RUN/'www/readonly/keep.txt').read_text()=='DO_NOT_DELETE','Deleted file'))
    test('405 includes mandatory Allow header', lambda: ensure('allow' in http('DELETE','/readonly/keep.txt')[1], '405 response has no Allow header'))
    test('GET directory autoindex', lambda: expect_http(200, path='/files/', contains=b'readme.txt'))
    test('GET missing file', lambda: expect_http(404, path='/files/absent.txt'))
    def redirect():
        result = expect_http(301,path='/old')
        ensure(result['headers'].get('location')=='/','Missing Location')
        return result
    test('301 redirect and Location', redirect)
    payload=bytes(range(256))*2
    test('Binary POST creates uploaded file', lambda: expect_http(201,'POST','/uploads/binary.bin',payload))
    test('upload_store differs from root', lambda: ensure((RUN/'www/storage/binary.bin').read_bytes()==payload and not (RUN/'www/uploads/binary.bin').exists(),'Wrong destination'))
    test('Retrieve binary upload', lambda: expect_http(200,path='/stored/binary.bin',exact=payload))
    test('Overwrite uploaded file', lambda: expect_http(200,'POST','/uploads/binary.bin',b'x'))
    test('Overwrite truncates correctly', lambda: expect_http(200,path='/stored/binary.bin',exact=b'x'))
    test('Second upload_store with trailing slash', lambda: expect_http(201,'POST','/ingest2/second.txt',b'second'))
    test('Second upload destination on disk', lambda: ensure((RUN/'www/storage2/second.txt').read_bytes()==b'second','Wrong storage2'))
    test('DELETE permitted', lambda: expect_http(204,'DELETE','/stored/binary.bin'))
    test('GET after DELETE', lambda: expect_http(404,path='/stored/binary.bin'))
    test('POST at exact location body limit', lambda: expect_http(201,'POST','/uploads/limit.bin',b'x'*1024))
    test('POST above location body limit', lambda: expect_http(413,'POST','/uploads/too-big.bin',b'x'*1025))
    test('POST inherits server body limit', lambda: expect_http(201,'POST','/inherit/inherited.bin',b'x'*2048))
    test('POST above server body limit', lambda: expect_http(413,'POST','/inherit/too-big.bin',b'x'*4097))
    test('Unknown method does not crash', lambda: expect_raw(400,[b'UNKNOWN / HTTP/1.1\r\nHost: localhost\r\n\r\n']))
    test('Unknown method uses 501 rather than 400', lambda: expect_raw(501,[b'UNKNOWN / HTTP/1.1\r\nHost: localhost\r\n\r\n']))
    test('Valid HEAD returns allowed-method response', lambda: expect_raw(405,[b'HEAD / HTTP/1.1\r\nHost: localhost\r\n\r\n']))
    test('Missing Host is rejected', lambda: expect_raw(400,[b'GET / HTTP/1.1\r\n\r\n']))
    test('Fragmented request', lambda: expect_raw(200,[b'GET / HTTP/1.1\r\nHo',b'st: localhost\r\n',b'\r\n']))
    test('CGI GET with query', lambda: expect_http(200,path='/cgi-bin/hello.py?evaluation=yes',contains=b'QUERY=evaluation=yes'))
    test('CGI POST', lambda: expect_http(200,'POST','/cgi-bin/hello.py',b'BODY IS HERE',contains=b'POST_BODY=BODY IS HERE'))
    test('CGI relative working directory', lambda: expect_http(200,path='/cgi-bin/relative.py',contains=b'RELATIVE_CGI_OK'))
    test('CGI headers and environment', lambda: expect_http(200,'POST','/cgi-bin/headers.py?x=1',b'abc',headers={'X-Evaluation':'present','Content-Type':'plain/text'},contains=b'HEADER=present\nCTYPE=plain/text\nLEN=3'))
    chunked=[b'POST /cgi-bin/hello.py HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n',b'5\r',b'\nhe',b'llo\r\n',b'0\r\n',b'\r\n']
    test('Incremental chunked CGI POST', lambda: expect_raw(200,chunked,contains=b'POST_BODY=hello'))
    test('CGI nonzero exit', lambda: expect_http(500,path='/cgi-bin/exit.py'))
    test('CGI syntax error', lambda: expect_http(500,path='/cgi-bin/syntax.py'))
    test('CGI invalid headers', lambda: expect_http(500,path='/cgi-bin/malformed.py'))
    test('CGI honors Status header', lambda: expect_http(201,path='/cgi-bin/status.py',contains=b'CREATED_CGI'))
    test('Server responds during infinite CGI', lambda: expect_http(200,contains=b'INDEX_CUSTOM'))
    test('curl commands from the checklist', curl_tests)
    test('Client resets release descriptors', lambda: reset_clients(process))
    test('Duplicate interface:port in configuration', duplicate_config)
    test('Two processes sharing interface:port', conflict_process)
    test('Real Chromium navigation and network capture', browser_tests)
    test('Nested autoindex link reaches correct resource', lambda: ensure(b'/files/sub/deep.txt' in http(path='/files/sub/')[2], 'Generated link uses /files/deep.txt instead of /files/sub/deep.txt'))
    test('Encoded filename from directory listing', lambda: expect_http(200,path='/files/a%20b.txt',contains=b'SPACE_FILE'))
    test('Siege 3 rounds: -b -c 5 -r 20', lambda: siege_tests(process))
    print('WAITING for real 60-second CGI and idle timeouts...', flush=True)
    pending_cgi.result()
    pending_idle.result()
    time.sleep(.6)
    test('Server still available after CGI timeout', lambda: expect_http(200,contains=b'INDEX_CUSTOM'))
    test('No CGI children after timeout', lambda: ensure(not psutil.Process(process.pid).children(),'CGI child still active'))
    test('No hanging client descriptors after completion', lambda: dict(fds=psutil.Process(process.pid).num_fds(),
         sockets=[dict(fd=c.fd,status=c.status) for c in psutil.Process(process.pid).connections()]))

def no_catchall():
    config=f'server {{ listen 127.0.0.1:{P1}; location /files {{ root www/files; allowed_methods GET; }} }}'
    with server(config,'no-catchall'):
        return expect_http(404,path='/unmatched')
test('No location /: unmatched URI is safe',no_catchall)

def valgrind():
    prefix=['valgrind','--leak-check=full','--show-leak-kinds=all','--errors-for-leak-kinds=definite,indirect','--error-exitcode=97', '--track-fds=yes', '--log-file='+str(LOG/'valgrind-%p.txt')]
    with server(CONFIG,'valgrind',prefix=prefix) as process:
        expect_http(200)
        expect_http(200,path='/files/')
        expect_http(404,path='/missing')
        expect_http(201,'POST','/uploads/valgrind.txt',b'abc')
        expect_http(200,path='/stored/valgrind.txt',exact=b'abc')
        expect_http(204,'DELETE','/stored/valgrind.txt')
        expect_http(200,path='/cgi-bin/hello.py')
        expect_http(200,'POST','/cgi-bin/hello.py',b'abc')
        expect_http(500,path='/cgi-bin/exit.py')
        pid=process.pid
    report=(LOG/f'valgrind-{pid}.txt').read_text()
    ensure(process.returncode==0, f'Valgrind exit={process.returncode}; see log')
    ensure('ERROR SUMMARY: 0 errors' in report, report[-1500:])
    ensure('All heap blocks were freed' in report or 'definitely lost: 0 bytes' in report,report[-1500:])
    return dict(exit=process.returncode,pid=pid,summary=report[-2500:])
test('Valgrind: requests, CGI and clean shutdown',valgrind)
save()
print('TOTAL',len(RESULTS),'PASS',sum(r['result']=='PASS' for r in RESULTS),'FAIL',sum(r['result']=='FAIL' for r in RESULTS),flush=True)
