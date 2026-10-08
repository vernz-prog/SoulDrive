import functools
import http.server
import json
import pathlib
import re
import sys
import threading
import time

from playwright.sync_api import sync_playwright

HERE = pathlib.Path(__file__).resolve().parent
WEB = HERE.parent.parent / 'web'
frames = [json.loads(l) for l in (HERE / 'frames.jsonl').read_text().splitlines()]
pick = [f for f in frames if f['spd'] > 20][:40] + [f for f in frames if f['l']['ce']][:20] + frames[-40:]
state = {'reset': 0, 'sent': None}

class Handler(http.server.SimpleHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def do_GET(self):
        if self.path != '/events':
            return super().do_GET()
        self.send_response(200)
        self.send_header('Content-Type', 'text/event-stream')
        self.send_header('Cache-Control', 'no-cache')
        self.end_headers()
        try:
            for f in pick:
                state['sent'] = f
                self.wfile.write(f'event: t\ndata: {json.dumps(f)}\n\n'.encode())
                self.wfile.flush()
                time.sleep(0.05)
            while True:
                time.sleep(1)
        except (BrokenPipeError, ConnectionResetError):
            pass

    def do_POST(self):
        if self.path == '/api/trip/reset':
            state['reset'] += 1
        self.send_response(204)
        self.end_headers()

srv = http.server.ThreadingHTTPServer(('127.0.0.1', 0), functools.partial(Handler, directory=str(WEB)))
threading.Thread(target=srv.serve_forever, daemon=True).start()
url = f'http://127.0.0.1:{srv.server_port}/index.html'

ok = fail = 0

def check(name, cond, detail=''):
    global ok, fail
    print(f"  {'OK  ' if cond else 'FAIL'}  {name:<58} {detail}")
    ok, fail = ok + bool(cond), fail + (not cond)

print('15. Сквозная проверка: прошивка → SSE → web/index.html')
html = (WEB / 'js' / 'app.js').read_text()
data_keys = set(re.findall(r"\bd\.(\w+)\b(?!\s*\()", html)) - {"l"}
check('все поля, которые читает экран, есть в JSON прошивки', data_keys <= set(frames[0]),
      f'нет: {sorted(data_keys - set(frames[0]))}' if data_keys - set(frames[0]) else ', '.join(sorted(data_keys)))

with sync_playwright() as p:
    b = p.chromium.launch()
    page = b.new_page(viewport={'width': 1280, 'height': 600})
    errors = []
    page.on('pageerror', lambda e: errors.append(str(e)))
    page.goto(url)
    page.wait_for_function("document.getElementById('status').textContent === 'ESP32'", timeout=5000)
    check('экран перешёл на данные платы (статус «ESP32», не ДЕМО)', True)
    used = set(page.evaluate("Object.keys(lampEls).concat(ALERTS.map(a => a[0]))"))
    lamp_keys = set(frames[0]['l'])
    check('все лампы и предупреждения экрана есть в JSON прошивки', used <= lamp_keys,
          f'нет: {sorted(used - lamp_keys)}' if used - lamp_keys else f'{len(used)} ключей')
    time.sleep(len(pick) * 0.05 + 1.5)
    f = state['sent']
    ui = page.evaluate("""() => ({
        spd: spdEl.textContent, vbat: $('vbatTop').textContent, trip: $('trip').textContent,
        odo: $('odo').textContent,
        lamps: Object.fromEntries(Object.entries(lampEls).map(([k, g]) => [k, g.classList.contains('on')])),
        clt: [...document.querySelectorAll('#cltScale rect.on')].length,
        fuel: [...document.querySelectorAll('#fuelScale rect.on')].length,
        alert: $('alert').classList.contains('on') ? $('alertL1').textContent + ' ' + $('alertL2').textContent : '' })""")
    check('скорость на экране = скорость из прошивки', ui['spd'] == str(round(f['spd'])), f"{ui['spd']} / {f['spd']}")
    check('напряжение', ui['vbat'] == f"{f['vbat']:.1f}", f"{ui['vbat']} / {f['vbat']}")
    check('суточный пробег', ui['trip'] == f"{f['trip']:.1f}", f"{ui['trip']} / {f['trip']}")
    check('общий пробег', int(ui['odo']) == int(f['odo']), f"{ui['odo']} / {f['odo']}")
    check('шкала ОЖ', ui['clt'] == round(min(max((f['clt'] - 50) / 80, 0), 1) * 8), f"{ui['clt']} из 8 сегментов, {f['clt']} °C")
    check('шкала топлива', ui['fuel'] == round(min(max(f['fuel'] / f['tank'], 0), 1) * 8), f"{ui['fuel']} из 8, {f['fuel']} л")
    wrong = [k for k, on in ui['lamps'].items() if on != bool(f['l'][k])]
    check('каждая лампа на экране совпадает с прошивкой', not wrong, f'расходятся: {wrong}' if wrong else f"горят: {[k for k, v in ui['lamps'].items() if v]}")
    check('карточка предупреждения соответствует лампам', bool(ui['alert']) == any(f['l'][k] for k in ('oil', 'chg', 'hot', 'brk', 'dr', 'sb', 'ce', 'res')), ui['alert'] or 'нет')
    page.dispatch_event('#trip', 'pointerdown')
    time.sleep(1.2)
    page.dispatch_event('#trip', 'pointerup')
    time.sleep(0.3)
    check('долгое нажатие на «A …» отправляет сброс суточного на плату', state['reset'] == 1, f"запросов: {state['reset']}")
    check('ошибок JavaScript нет', not errors, '; '.join(errors))
    page.screenshot(path=str(HERE / 'e2e_screen.png'))
    b.close()

print(f'\nСквозная проверка: {ok} пройдено, {fail} не пройдено')
sys.exit(1 if fail else 0)
