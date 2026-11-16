import ast
import concurrent.futures
import json
from pathlib import Path
import socket
import time

BASE=Path('/tmp/webserv-evaluation-2026-11-16')
tree=ast.parse((BASE/'evaluate.py').read_text())
definitions=ast.Module(body=[n for n in tree.body if isinstance(n,(ast.Import,ast.ImportFrom,ast.FunctionDef))],type_ignores=[])
endpoints=json.loads((BASE/'endpoints.json').read_text())
ns={'P1':endpoints['main']}
exec(compile(definitions,str(BASE/'evaluate.py'),'exec'),ns)
results=json.loads((BASE/'results.json').read_text())
superseded=[r for r in results if r['name'] in ('Siege 3 rounds: -b -c 5 -r 20','No hanging client descriptors after completion')]
(BASE/'superseded-harness-checks.json').write_text(json.dumps(superseded,indent=2))
results=[r for r in results if r not in superseded]
for r in results:
    if r['name']=='Idle partial request closes within 60 seconds':
        r['name']='Idle partial request is closed (60-second configured timeout)'
ns.update(BASE=BASE,REPO=Path('/home/kjroydev/proyectos/webserv'),RUN=BASE/'run',LOG=BASE/'logs',
          P1=endpoints['main'],P2=endpoints['second'],RESULTS=results,
          RESPONSES=json.loads((BASE/'responses.json').read_text()))
test=ns['test']
expect_http=ns['expect_http']
ensure=ns['ensure']


def descriptors(process):
    time.sleep(.25)
    proc=ns['psutil'].Process(process.pid)
    sockets=[dict(fd=c.fd,status=c.status) for c in proc.connections()]
    ensure(all(c['status']=='LISTEN' for c in sockets),f'Non-listening sockets remain: {sockets}')
    ensure(len(sockets)==3,f'Expected three listeners, got {sockets}')
    ensure(not proc.children(),'CGI child left behind')
    return dict(fds=proc.num_fds(),sockets=sockets,children=0)


def chunk_probe():
    headers=b'POST /cgi-bin/hello.py HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n'
    cases={'whole':[headers+b'5\r\nhello\r\n0\r\n\r\n'],
           'fragmented':[headers,b'5\r',b'\nhe',b'llo\r\n',b'0\r\n',b'\r\n']}
    observed={}
    for name,parts in cases.items():
        with socket.create_connection(('127.0.0.1',endpoints['main']),timeout=5) as client:
            client.settimeout(5)
            send_error=None
            for part in parts:
                try:
                    client.sendall(part)
                    time.sleep(.02)
                except OSError as exc:
                    send_error=str(exc)
                    break
            output=b''
            while True:
                try:
                    block=client.recv(65536)
                except ConnectionResetError:
                    break
                if not block: break
                output+=block
        observed[name]=dict(parts=[p.decode() for p in parts],response=output.decode(errors='replace'),send_error=send_error)
    (BASE/'chunked-reproduction.json').write_text(json.dumps(observed,indent=2))
    ensure(observed['whole']['response'].startswith('HTTP/1.1 200'),'Whole request failed')
    ensure(observed['fragmented']['response'].startswith('HTTP/1.1 200'),
           'Fragmented chunked request rejected: '+observed['fragmented']['response'][:200])
    return observed


def disk_progress():
    blob=b'0123456789abcdef'*65536
    (BASE/'run/www/blob.bin').write_bytes(blob)
    with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
        large=pool.submit(expect_http,200,path='/blob.bin',exact=blob)
        latency=[]
        for _ in range(8):
            start=time.monotonic()
            expect_http(200,contains=b'INDEX_CUSTOM')
            latency.append(round((time.monotonic()-start)*1000,3))
        large.result(timeout=10)
    ensure(max(latency)<1000,f'Stall observed: {latency}')
    return dict(file_bytes=len(blob),small_requests=8,latency_ms=latency)


config=(BASE/'run/main.conf').read_text()
with ns['server'](config,'followup') as process:
    # Repeat only checks whose instrumentation needed correction.
    test('Siege 3 rounds: -b -c 5 -r 20',lambda:ns['siege_tests'](process))
    test('No hanging client descriptors after completion',lambda:descriptors(process))
    test('Reproduce whole versus fragmented chunked CGI request',chunk_probe)
    test('Regular-file read and concurrent small GET progress',disk_progress)
    test('GET empty file returns 200 with zero bytes',lambda:expect_http(200,path='/empty.txt',exact=b''))
ns['save']()
print('FINAL',len(results),'PASS',sum(r['result']=='PASS' for r in results),'FAIL',sum(r['result']=='FAIL' for r in results))
