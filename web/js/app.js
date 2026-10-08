'use strict';
const NS = 'http://www.w3.org/2000/svg';
const $ = id => document.getElementById(id);
function el(tag, attrs, parent, text) {
  const e = document.createElementNS(NS, tag);
  for (const k in attrs) e.setAttribute(k, attrs[k]);
  if (text !== undefined) e.textContent = text;
  if (parent) parent.appendChild(e);
  return e;
}

const GEARS = [3.636, 1.95, 1.357, 0.941, 0.784];
const FINAL_DRIVE = 3.9;
const WHEEL_M = Math.PI * (13 * 25.4 + 2 * 0.7 * 175) / 1000;

const ICONS = window.SOUL_ICONS;
function icon(parent, name, size, color, cls) {
  const g = el('g', { class: 'ico ' + (cls || '') }, parent);
  g.style.color = color;
  el('svg', { width: size, height: size, viewBox: '0 0 32 32' }, g).innerHTML = ICONS[name];
  return g;
}

const lampEls = {};
function lamp(id, parent, name, color, x, y) {
  const g = icon(parent, name, 34, color, 'lamp');
  g.setAttribute('transform', `translate(${x} ${y})`);
  lampEls[id] = g;
}
lamp('tl', $('turnL'), 'turnL', 'var(--green)', 0, 0);
lamp('tr', $('turnR'), 'turnR', 'var(--green)', 0, 0);
[['pk', 'park', 'var(--green)'], ['lb', 'low', 'var(--green)'], ['hb', 'high', 'var(--blue)'],
 ['fg', 'fogR', 'var(--yellow)']]
  .forEach(([id, ic, c], i) => lamp(id, $('lampsL'), ic, c, 112 + i * 46, 526));
[['chk', 'choke', 'var(--yellow)'], ['res', 'fuel', 'var(--yellow)'], ['ce', 'engine', 'var(--yellow)'],
 ['sb', 'belt', 'var(--red)'], ['dr', 'door', 'var(--red)'], ['chg', 'batt', 'var(--red)'],
 ['oil', 'oil', 'var(--red)'], ['brk', 'brake', 'var(--red)'], ['hot', 'temp', 'var(--red)']]
  .forEach(([id, ic, c], i) => lamp(id, $('lampsR'), ic, c, 800 + i * 42, 526));

icon($('cltIcon'), 'temp', 28, '#eef1f4');
icon($('fuelIcon'), 'fuel', 28, '#eef1f4');

const A0 = -130, A1 = 130;
function polar(cx, cy, r, deg) {
  const a = (deg - 90) * Math.PI / 180;
  return [cx + r * Math.cos(a), cy + r * Math.sin(a)];
}
function arcPath(cx, cy, r, a1, a2) {
  const [x1, y1] = polar(cx, cy, r, a1), [x2, y2] = polar(cx, cy, r, a2);
  return `M${x1} ${y1}A${r} ${r} 0 ${a2 - a1 > 180 ? 1 : 0} 1 ${x2} ${y2}`;
}
function sectorPath(cx, cy, r1, r2, a1, a2) {
  const [x1, y1] = polar(cx, cy, r2, a1), [x2, y2] = polar(cx, cy, r2, a2);
  const [x3, y3] = polar(cx, cy, r1, a2), [x4, y4] = polar(cx, cy, r1, a1);
  return `M${x1} ${y1}A${r2} ${r2} 0 0 1 ${x2} ${y2}L${x3} ${y3}A${r1} ${r1} 0 0 0 ${x4} ${y4}Z`;
}

const SEG_W = 140, SEG_SAG_R = 228;
const SEG_PATHS = [
  'M4.55798 10.8683C21.6262 2.53329 45.7434 -0.0710241 69.6617 0.00152118L69.6622 5.09701L62.1441 5.29061L60.9112 11.5048C60.7503 12.3158 60.4602 13.7757 57.5664 13.609C45.9218 12.9377 23.0702 18.9492 13.9597 21.6222C9.86626 22.8231 6.76027 23.1945 5.70469 22.4408C4.38357 21.618 2.10315 18.002 0.828226 15.5987C-0.298787 13.6497 -0.0991403 13.0149 0.391933 12.7713C1.67778 12.1338 2.9707 11.5804 3.84016 11.1959L4.55798 10.8683ZM55.7821 5.60134C47.3613 5.5297 26.6212 7.77857 7.98234 14.7836C7.50178 14.9642 7.1512 15.2774 6.96564 15.6817C6.78321 16.0793 6.78235 16.5163 6.90179 16.9123C7.13916 17.6982 7.86708 18.3964 8.87141 18.5868C9.10682 18.6313 9.47807 18.6302 9.90542 18.6016C10.3503 18.5718 10.9072 18.5087 11.5414 18.4111C12.8105 18.2159 14.4088 17.8772 16.07 17.3651C26.2864 14.2155 48.5945 10.6615 55.8406 9.96459C57.0589 9.84725 57.6248 8.79096 57.6288 7.84854C57.6325 6.90038 57.0802 5.83894 55.8713 5.60975L55.8271 5.60212L55.7821 5.60134Z',
  'M134.748 10.8682C117.68 2.53328 93.5627 -0.0710516 69.6444 0.0014659L69.644 5.09696L77.162 5.29153L78.3949 11.5057C78.5559 12.3168 78.8461 13.7758 81.7397 13.6089C93.3845 12.9377 116.236 18.9502 125.346 21.6231C129.44 22.8241 132.546 23.1947 133.601 22.4407C134.923 21.6178 137.203 18.0028 138.478 15.5997C139.605 13.6508 139.405 13.0149 138.914 12.7713C137.628 12.1337 136.335 11.5803 135.466 11.1958L134.748 10.8682ZM83.524 5.60129C91.9448 5.52962 112.686 7.77837 131.325 14.7836C131.805 14.9642 132.155 15.2783 132.34 15.6826C132.523 16.0802 132.525 16.5163 132.405 16.9123C132.168 17.6983 131.439 18.3973 130.435 18.5877C130.199 18.6322 129.829 18.6301 129.402 18.6016C128.957 18.5718 128.4 18.5096 127.766 18.4121C126.497 18.2169 124.898 17.8772 123.237 17.3651C113.021 14.2154 90.7117 10.6614 83.4656 9.96454C82.2473 9.84706 81.6813 8.79091 81.6774 7.84849C81.6736 6.90042 82.2262 5.84 83.4349 5.61067L83.4791 5.60206L83.524 5.60129Z',
].map(d => d.match(/[MLCZ]|-?\d*\.?\d+(?:e-?\d+)?/gi));
const SEG_K = .85;
const SEG_GAP = 5;

function warpSegment(cx, cy, rTop, a1, a2) {
  const map = (x, y) => {
    const dx = x - SEG_W / 2;
    const sag = SEG_SAG_R - Math.sqrt(SEG_SAG_R * SEG_SAG_R - dx * dx);
    const [px, py] = polar(cx, cy, rTop - (y - sag) * SEG_K, a1 + (a2 - a1) * x / SEG_W);
    return px.toFixed(2) + ' ' + py.toFixed(2);
  };
  let out = '';
  for (const tok of SEG_PATHS) {
    for (let i = 0; i < tok.length;) {
      const c = tok[i++];
      if (c === 'Z' || c === 'z') { out += 'Z'; continue; }
      const n = c === 'C' ? 3 : 1;
      const pts = [];
      for (let k = 0; k < n; k++) { pts.push(map(+tok[i], +tok[i + 1])); i += 2; }
      out += c + pts.join(' ');
    }
  }
  return out;
}

function makeGauge(root, o) {
  const { cx, cy, R, max, major, labelDiv, redFrom, unitLabel } = o;
  const ang = v => A0 + (A1 - A0) * Math.min(Math.max(v, 0), max) / max;
  const rDisk = R * .64;
  const rIn = rDisk + 4, rOut = R - 13;

  el('circle', { cx, cy, r: R - 11, fill: 'url(#ring)' }, root);

  const fan = el('g', { filter: 'url(#fanBlur)' }, root);
  const N = 64, step = (A1 - A0) / N;
  const wedges = [];
  for (let i = 0; i < N; i++) {
    const a = A0 + i * step;
    wedges.push({ a: a + step, p: el('path', { d: sectorPath(cx, cy, rIn, rOut, a, a + step),
      fill: '#fff', opacity: 0, 'shape-rendering': 'crispEdges' }, fan) });
  }

  const segs = el('g', { filter: 'url(#segGlow)' }, root);
  const gapDeg = SEG_GAP / (R + 6) * 180 / Math.PI;
  for (let v = 0; v < max - 1e-6; v += major) {
    const color = redFrom && v >= redFrom ? 'var(--red)' : 'var(--scale)';
    el('path', { d: warpSegment(cx, cy, R + 6, ang(v) + gapDeg, ang(v + major) - gapDeg), fill: color,
      'fill-rule': 'evenodd' }, segs);
    const mid = ang(v + major / 2);
    const [x1, y1] = polar(cx, cy, R + 4, mid), [x2, y2] = polar(cx, cy, R - 6, mid);
    el('line', { x1, y1, x2, y2, stroke: '#f2f4f7', 'stroke-width': 2, 'stroke-linecap': 'round' }, root);
  }
  if (redFrom) {
    el('path', { d: arcPath(cx, cy, R, ang(redFrom), ang(max)), fill: 'none', stroke: 'var(--red)',
      'stroke-width': 10, opacity: .35, filter: 'url(#blur3)' }, root);
  }

  for (let v = 0; v <= max + 1e-6; v += major) {
    const red = redFrom && v >= redFrom;
    const [x1, y1] = polar(cx, cy, R - 11, ang(v)), [x2, y2] = polar(cx, cy, R + 12, ang(v));
    el('line', { x1, y1, x2, y2, class: 'tick' + (red ? ' red' : '') }, root);
    const [lx, ly] = polar(cx, cy, R + 33, ang(v));
    el('text', { x: lx, y: ly, class: 'scaleNum' + (red ? ' red' : ''),
      transform: `rotate(${ang(v)} ${lx} ${ly})` }, root, v / labelDiv);
  }
  const [zx, zy] = polar(cx, cy, R + 33, ang(0));
  el('text', { x: zx + 14, y: zy + 26, 'font-size': 12, class: 'muted' }, root, unitLabel);

  const needle = el('g', {}, root);
  const nb = cy - rDisk - 6, nt = cy - R + 3;
  const wedge = (wb, wt) => `M${cx - wb} ${nb}L${cx - wt} ${nt}Q${cx} ${nt - wt} ${cx + wt} ${nt}` +
    `L${cx + wb} ${nb}Q${cx} ${nb + wb * .8} ${cx - wb} ${nb}Z`;
  el('path', { d: wedge(12, 5), fill: 'var(--orange)', opacity: .8, filter: 'url(#blur6)' }, needle);
  el('path', { d: wedge(7.5, 2), fill: 'url(#needleGrad)' }, needle);
  el('path', { d: wedge(1.6, .6), fill: 'url(#needleShine)' }, needle);

  el('circle', { cx, cy, r: rDisk, fill: 'none', stroke: 'var(--orange)', 'stroke-width': 16,
    opacity: .45, filter: 'url(#blur6)' }, root);
  el('circle', { cx, cy, r: rDisk, fill: 'url(#disk)', stroke: 'url(#orangeRing)', 'stroke-width': 4.5 }, root);
  const center = el('g', {}, root);

  let shown = 0;
  return {
    center,
    set(v) {
      shown += (v - shown) * .35;
      const a = ang(shown);
      needle.setAttribute('transform', `rotate(${a} ${cx} ${cy})`);
      for (const w of wedges) {
        const d = a - w.a;
        const op = d < -step ? 0 : .05 + .62 * Math.exp(-Math.max(d, 0) / 30);
        w.p.setAttribute('opacity', op.toFixed(3));
      }
    },
  };
}

const tacho = makeGauge($('tacho'), { cx: 290, cy: 262, R: 178, max: 7000, major: 1000, labelDiv: 1000,
  redFrom: 6000, unitLabel: 'x1000 rpm' });
const speedo = makeGauge($('speedo'), { cx: 990, cy: 262, R: 178, max: 220, major: 20, labelDiv: 1,
  unitLabel: 'km/h' });

el('circle', { cx: 262, cy: 262, r: 20, fill: 'none', stroke: '#eef1f4', 'stroke-width': 2 }, tacho.center);
icon(tacho.center, 'shift', 26, '#eef1f4').setAttribute('transform', 'translate(249 249)');
const gearEl = el('text', { x: 318, y: 264, 'font-size': 50, 'text-anchor': 'middle',
  'dominant-baseline': 'central' }, tacho.center, 'N');
const spdEl = el('text', { x: 990, y: 268, 'font-size': 82, 'text-anchor': 'middle',
  'dominant-baseline': 'central' }, speedo.center, '0');
el('text', { x: 990, y: 318, 'font-size': 15, 'text-anchor': 'middle', class: 'muted' }, speedo.center, 'km/h');

function makeBar(root, x, labels, redSeg) {
  const n = 8, w = 30, gap = 4, y = 478;
  const segs = [];
  for (let i = 0; i < n; i++) {
    segs.push(el('rect', { x: x + i * (w + gap), y, width: w, height: 8,
      class: 'seg' + (i === redSeg ? ' redOff' : '') }, root));
  }
  for (const [i, txt, red] of labels) {
    const t = el('text', { x: x + i * (w + gap) + w / 2, y: 468, 'font-size': 17, 'text-anchor': 'middle' }, root, txt);
    if (red) t.style.fill = 'var(--red)';
  }
  return (frac, redFn) => {
    const lit = Math.round(Math.min(Math.max(frac, 0), 1) * n);
    segs.forEach((s, i) => {
      s.classList.toggle('on', i < lit);
      s.classList.toggle('red', redFn(i));
    });
  };
}
const setClt = makeBar($('cltScale'), 152, [[0, '50'], [3.5, '90'], [7, '130', true]], 7);
const setFuel = makeBar($('fuelScale'), 856, [[0, '0', true], [3.5, '1/2'], [7, '1']], 0);

for (let i = 0; i < 7; i++) {
  const y = 380 + i * 8, half = 120 - i * 13;
  el('path', { d: `M${640 - half} ${y}Q640 ${y + 6} ${640 + half} ${y}`, fill: 'none',
    stroke: 'url(#divider)', 'stroke-width': 1, opacity: .6 - i * .07 }, $('funnel'));
}

const PAGES = [
  { menu: 'Поездка', title: 'С момента запуска', rows: [['road', 'sDist', 'km'], ['clock', 'sTime', 'h'],
      ['gauge', 'sAvg', 'km/h', 'Ср.'], ['flag', 'sMax', 'km/h', 'Макс.']],
    hint: ['Для сброса нажмите', 'и удерживайте экран'] },
  { menu: 'Состояние автомобиля', title: 'Состояние автомобиля', rows: [['temp', 'eClt', '°C'],
      ['fuel', 'eFuel', 'L'], ['gauge', 'eRpm', 'rpm']] },
  { menu: 'Напряжение сети', title: 'Напряжение сети', rows: [['batt', 'eVolt', 'V']], note: 'eCharge' },
  { menu: 'Пробег', title: 'Пробег', rows: [['road', 'tA', 'km', 'A'], ['odo', 'tOdo', 'km']],
    hint: ['Сброс A — удерживайте', 'значение внизу экрана'] },
  { menu: 'Карта', map: true, rows: [] },
];
const pageEls = [], val = {};
PAGES.forEach(p => {
  const g = el('g', { class: 'view' }, $('pages'));
  el('text', { x: 640, y: 118, 'font-size': 17, 'text-anchor': 'middle' }, g, p.title);
  const rowH = p.rows.length > 3 ? 40 : 46;
  p.rows.forEach(([ic, id, unit, prefix], i) => {
    const y = 160 + i * rowH;
    if (prefix) el('text', { x: 584, y: y, 'font-size': 13, 'text-anchor': 'end', class: 'muted' }, g, prefix);
    icon(g, ic, 22, '#eef1f4').setAttribute('transform', `translate(592 ${y - 17})`);
    const t = el('text', { x: 626, y: y + 1, 'font-size': 21 }, g);
    val[id] = el('tspan', {}, t, '---');
    el('tspan', { dx: 4, 'font-size': 13 }, t, unit);
  });
  if (p.note) val[p.note] = el('text', { x: 640, y: 228, 'font-size': 13, 'text-anchor': 'middle',
    class: 'muted' }, g, '');
  (p.hint || []).forEach((h, i) => el('text', { x: 640, y: 334 + i * 15, 'font-size': 11.5,
    'text-anchor': 'middle' }, g, h));
  pageEls.push(g);
});

const menuEl = $('menu');
el('path', { d: 'M628 100l12-9 12 9', fill: 'none', stroke: '#eef1f4', 'stroke-width': 2.5 }, menuEl);
el('path', { d: 'M628 344l12 9 12-9', fill: 'none', stroke: '#eef1f4', 'stroke-width': 2.5 }, menuEl);
const menuSel = el('g', {}, menuEl);
const MENU_Y0 = 136, MENU_DY = 44;
el('rect', { x: 512, y: -17, width: 256, height: 34, fill: 'var(--orange)', opacity: .5, filter: 'url(#blur6)' }, menuSel);
el('rect', { x: 512, y: -17, width: 256, height: 34, fill: 'url(#menuSel)' }, menuSel);
el('path', { d: 'M508 114H772', stroke: 'url(#divider)', 'stroke-width': 1.2 }, menuEl);
PAGES.forEach((p, i) => {
  const y = MENU_Y0 + i * MENU_DY;
  el('path', { d: `M508 ${y + MENU_DY / 2}H772`, stroke: 'url(#divider)', 'stroke-width': 1.2 }, menuEl);
  el('text', { x: 640, y: y + 1, 'font-size': 15, 'text-anchor': 'middle', 'dominant-baseline': 'central' },
    menuEl, p.menu);
});

let page = 0, menuTimer = null;
function show(view) {
  menuEl.classList.toggle('on', view === 'menu');
  pageEls.forEach((g, j) => g.classList.toggle('on', view === 'page' && j === page));
  mapMode(view === 'page' && !!PAGES[page].map);
}

let mapK = 0, mapTarget = 0, mapAnim = null;
function mapMode(on) {
  document.body.classList.toggle('map', on);
  mapTarget = on ? 1 : 0;
  if (mapAnim) return;
  let prev = performance.now();
  const frame = now => {
    const dk = (now - prev) / 700;
    prev = now;
    mapK = mapTarget > mapK ? Math.min(mapTarget, mapK + dk) : Math.max(mapTarget, mapK - dk);
    const e = mapK < .5 ? 2 * mapK * mapK : 1 - 2 * (1 - mapK) * (1 - mapK);
    const sc = 1 - .26 * e;
    $('tacho').setAttribute('transform', `translate(${-9.6 * e} ${61.1 * e}) scale(${sc})`);
    $('speedo').setAttribute('transform', `translate(${342.4 * e} ${61.1 * e}) scale(${sc})`);
    $('nav').style.opacity = Math.max(0, e * 1.4 - .4);
    $('funnel').style.opacity = 1 - e;
    mapAnim = mapK === mapTarget ? null : requestAnimationFrame(frame);
  };
  mapAnim = requestAnimationFrame(frame);
}
function step(dir) {
  page = (page + dir + PAGES.length) % PAGES.length;
  menuSel.setAttribute('transform', `translate(0 ${MENU_Y0 + page * MENU_DY})`);
  show('menu');
  clearTimeout(menuTimer);
  menuTimer = setTimeout(() => show('page'), 2500);
}
const params = new URLSearchParams(location.search);
if (params.has('map')) page = PAGES.findIndex(p => p.map);
menuSel.setAttribute('transform', `translate(0 ${MENU_Y0 + page * MENU_DY})`);
show('page');

const ALERTS = [
  ['oil', 'oil', 'Низкое давление', 'масла!', '#e01b1b'],
  ['chg', 'batt', 'Нет заряда', 'АКБ', '#e01b1b'],
  ['hot', 'temp', 'Перегрев', 'двигателя', '#e01b1b'],
  ['brk', 'brake', 'Ручной тормоз /', 'уровень ТЖ', '#e01b1b'],
  ['dr', 'door', 'Открыта', 'дверь', '#e01b1b'],
  ['sb', 'belt', 'Пристегните', 'ремень', '#e01b1b'],
  ['ce', 'engine', 'Проверьте датчики', 'и бортсеть', '#a06a00'],
  ['res', 'fuel', 'Мало', 'топлива', '#a06a00'],
];
let alertFor = null;
function updateAlert(l) {
  const a = ALERTS.find(([id]) => l[id]);
  $('alert').classList.toggle('on', !!a);
  $('pages').style.opacity = a ? 0 : 1;
  if (!a || alertFor === a[0]) return;
  alertFor = a[0];
  $('alertIcon').innerHTML = '';
  icon($('alertIcon'), a[1], 44, a[4]);
  $('alertL1').textContent = a[2];
  $('alertL2').textContent = a[3];
}

const nav = (() => {
  const H = [[0, 'ул. Мира'], [400, 'ул. Ленина'], [800, 'Садовая ул.'], [1200, 'ул. Гагарина'],
             [1600, 'Заводская ул.']];
  const V = [[0, 'Школьная ул.'], [500, 'пр. Победы'], [1000, 'Советская ул.'], [1500, 'Молодёжная ул.'],
             [2000, 'Полевая ул.']];
  const X0 = -700, X1 = 2700, Y0 = -600, Y1 = 2200;
  const ROUTE = [[0, 1600, 'Школьная ул.', 60], [0, 400, 'ул. Ленина', 60], [1000, 400, 'Советская ул.', 40],
    [1000, 0, 'ул. Мира', 60], [2000, 0, 'Полевая ул.', 90], [2000, 1200, 'ул. Гагарина', 60],
    [500, 1200, 'пр. Победы', 60], [500, 1600, 'Заводская ул.', 40]];

  const world = $('world');
  let seed = 7;
  const rnd = () => (seed = seed * 16807 % 2147483647) / 2147483647;

  const blocks = el('g', { fill: '#121b2a' }, world);
  for (let x = X0; x < X1; x += 250) {
    for (let y = Y0; y < Y1; y += 200) {
      if (x >= 1000 && x < 1500 && y >= 400 && y < 800) continue;
      const n = 1 + Math.floor(rnd() * 3);
      for (let k = 0; k < n; k++) {
        const w = 50 + rnd() * 120, h = 40 + rnd() * 90;
        el('rect', { x: x + 25 + rnd() * (200 - w), y: y + 25 + rnd() * (150 - h),
          width: w, height: h, rx: 6 }, blocks);
      }
    }
  }
  el('rect', { x: 1030, y: 430, width: 440, height: 340, rx: 30, fill: '#0f2a1d' }, world);
  el('path', { d: 'M-700 1950C0 1850 300 2050 900 1900S1900 1500 2200 1000S2400 0 2700 -300', fill: 'none',
    stroke: '#10314f', 'stroke-width': 110, 'stroke-linecap': 'round' }, world);

  const minor = el('g', { stroke: '#1d2a3d', 'stroke-width': 9 }, world);
  for (let x = X0 + 250; x < X1; x += 500) {
    for (let y = Y0; y < Y1; y += 400) if (rnd() > .25) el('path', { d: `M${x} ${y}V${y + 400}` }, minor);
  }
  for (let y = Y0 + 200; y < Y1; y += 400) {
    for (let x = X0; x < X1; x += 500) if (rnd() > .25) el('path', { d: `M${x} ${y}H${x + 500}` }, minor);
  }
  const major = el('g', { stroke: '#2b3b54', 'stroke-width': 20, 'stroke-linecap': 'round' }, world);
  H.forEach(([y]) => el('path', { d: `M${X0} ${y}H${X1}` }, major));
  V.forEach(([x]) => el('path', { d: `M${x} ${Y0}V${Y1}` }, major));

  const pts = ROUTE.concat([ROUTE[0]]);
  const segs = [];
  let L = 0;
  for (let i = 0; i < ROUTE.length; i++) {
    const [x1, y1] = pts[i], [x2, y2] = pts[i + 1], len = Math.hypot(x2 - x1, y2 - y1);
    segs.push({ x1, y1, dx: (x2 - x1) / len, dy: (y2 - y1) / len, len, from: L,
      street: ROUTE[i][2], limit: ROUTE[i][3] });
    L += len;
  }
  const d = 'M' + pts.map(p => p[0] + ' ' + p[1]).join('L');
  el('path', { d, fill: 'none', stroke: '#5b3a2a', 'stroke-width': 12, 'stroke-linejoin': 'round' }, world);
  const ahead = [
    el('path', { d, fill: 'none', stroke: '#ff6a26', 'stroke-width': 34, opacity: .35,
      'stroke-linejoin': 'round' }, world),
    el('path', { d, fill: 'none', stroke: '#ff8a45', 'stroke-width': 14, 'stroke-linejoin': 'round' }, world),
  ];

  const labels = el('g', { id: 'navLabels' }, world);
  const labelEls = [];
  const label = (x, y, txt, cls) => labelEls.push(el('text', { x, y, class: cls || '' }, labels, txt));
  H.forEach(([y, n], i) => [-450 + i * 130, 1250 + i * 90].forEach(x => label(x, y - 32, n)));
  V.forEach(([x, n], i) => [100 + i * 110, 1000 + i * 70].forEach(y => label(x + 30, y, n)));
  label(1250, 600, 'Парк', 'area');
  label(250, 2050, 'ЗАРЕЧЬЕ', 'area');
  label(2300, 1700, 'ПРОМЗОНА', 'area');
  label(750, 1000, 'ЦЕНТР', 'area');

  const arrow = $('turnArrow').children;
  const ARROWS = {
    straight: ['M0 12V-10', 'M-7 -3L0 -10L7 -3'],
    right: ['M-6 12V2Q-6 -6 2 -6H9', 'M3 -13L10 -6L3 1'],
    left: ['M6 12V2Q6 -6 -2 -6H-9', 'M-3 -13L-10 -6L-3 1'],
  };
  const SIGN_DIGITS = [
    'M0 4A4 4 0 0 1 8 4V10A4 4 0 0 1 0 10Z',
    'M1.2 3.4L5.4 0V14',
    'M0.2 3.8A3.9 3.8 0 0 1 7.8 3.8C7.8 6 6.6 7.4 5 8.8L0 13.9H8.2',
    'M0 3.3A4 3.3 0 1 1 4 6.6A4 3.7 0 1 1 0 10.6',
    'M6 14V0L0 9.8H8.4',
    'M7.6 0H0.9L0.4 6.3C1.4 5.5 2.6 5.1 4 5.1C6.4 5.1 8 7 8 9.5C8 12.2 6.3 14 4 14C2 14 0.6 12.9 0 11.2',
    'M7.4 1.5C6.6 0.5 5.4 0 4 0C1.6 0 0 2 0 5V9.8M0 9.8A4 4.2 0 1 0 8 9.8A4 4.2 0 1 0 0 9.8',
    'M0 0H8L3 14',
    'M4 6.8A3.6 3.4 0 1 1 4 0A3.6 3.4 0 1 1 4 6.8A4 3.6 0 1 0 4 14A4 3.6 0 1 0 4 6.8Z',
    'M8 4.2A4 4.2 0 1 0 0 4.2A4 4.2 0 1 0 8 4.2V9C8 12 6.4 14 4 14C2.6 14 1.4 13.5 0.6 12.5',
  ];
  const limitEl = $('limit');
  let shownLimit = null;
  function setLimit(n) {
    if (n === shownLimit) return;
    shownLimit = n;
    limitEl.innerHTML = '';
    const str = String(n), adv = 11.2;
    const w = (str.length - 1) * adv + 8;
    const k = str.length > 2 ? .82 : 1.1;
    const g = el('g', { transform: `translate(792 121) scale(${k}) translate(${-w / 2} -7)` }, limitEl);
    [...str].forEach((c, i) => el('path', { d: SIGN_DIGITS[+c], transform: `translate(${i * adv} 0)` }, g));
  }

  const fmtDist = m => m < 1000 ? Math.max(10, Math.round(m / 10) * 10) + ' m' : (m / 1000).toFixed(1) + ' km';

  let pos = 0, heading = null, scale = .45, lastT = null, lastFont = 0;
  const api = { limit: 60 };
  api.skip = m => { pos = (pos + m) % L; };
  api.update = spd => {
    const now = performance.now();
    const dt = lastT === null ? 0 : Math.min((now - lastT) / 1000, .25);
    lastT = now;
    pos = (pos + spd / 3.6 * dt) % L;

    const i = segs.findIndex(s => pos < s.from + s.len);
    const s = segs[i], nx = segs[(i + 1) % segs.length];
    const t = pos - s.from;
    const px = s.x1 + s.dx * t, py = s.y1 + s.dy * t;

    const target = Math.atan2(s.dx, -s.dy) * 180 / Math.PI;
    if (heading === null) heading = target;
    heading += ((target - heading + 540) % 360 - 180) * Math.min(1, dt * 3);
    scale += (.5 - .22 * Math.min(spd / 100, 1) - scale) * Math.min(1, dt * 1.5);

    world.setAttribute('transform',
      `translate(640 336) rotate(${-heading}) scale(${scale}) translate(${-px} ${-py})`);
    for (const p of ahead) p.setAttribute('stroke-dasharray', `0 ${pos} ${L}`);
    const font = Math.round(13 / scale);
    if (font !== lastFont) { labels.setAttribute('font-size', font); lastFont = font; }
    for (const e of labelEls) e.setAttribute('transform', `rotate(${heading} ${e.getAttribute('x')} ${e.getAttribute('y')})`);

    const toTurn = s.len - t;
    const cross = s.dx * nx.dy - s.dy * nx.dx;
    const dir = toTurn > 1500 ? 'straight' : cross > 0 ? 'right' : 'left';
    ARROWS[dir].forEach((p, k) => arrow[k].setAttribute('d', p));
    $('turnDist').textContent = fmtDist(toTurn);
    $('turnStreet').textContent = nx.street;
    api.limit = s.limit;
    setLimit(s.limit);

    const left = L - pos, min = Math.round(left / 1000 / 35 * 60);
    $('navLeft').textContent = (left / 1000).toFixed(1);
    const eta = new Date(Date.now() + min * 60000);
    $('navEta').textContent = eta.getHours() + ':' + String(eta.getMinutes()).padStart(2, '0');
    $('navTime').textContent = min < 60 ? min + ' мин' : Math.floor(min / 60) + ' ч ' + min % 60 + ' мин';
  };
  return api;
})();

const session = { t0: Date.now(), odo0: null, max: 0 };
function resetSession(odo) { session.t0 = Date.now(); session.odo0 = odo; session.max = 0; }

function estimateGear(rpm, spd) {
  if (spd < 3 || rpm < 700) return 'N';
  const kmhPerRpm = spd / rpm;
  let best = null, err = Infinity;
  GEARS.forEach((g, i) => {
    const expected = WHEEL_M * 60 / (g * FINAL_DRIVE * 1000);
    const e = Math.abs(kmhPerRpm - expected) / expected;
    if (e < err) { err = e; best = i + 1; }
  });
  return err < 0.15 ? String(best) : '–';
}

const allLamps = params.has('lamps');
let last = null;

function render(d) {
  last = d;
  tacho.set(d.rpm);
  speedo.set(d.spd);
  spdEl.textContent = Math.round(d.spd);
  gearEl.textContent = estimateGear(d.rpm, d.spd);

  $('vbatTop').textContent = d.vbat.toFixed(1);
  setClt((d.clt - 50) / 80, i => i === 7 || d.l.hot);
  setFuel(d.fuel / d.tank, i => i === 0 && d.l.res);
  $('trip').textContent = d.trip.toFixed(1);
  $('odo').textContent = String(Math.floor(d.odo)).padStart(6, '0');

  if (session.odo0 === null) session.odo0 = d.odo;
  session.max = Math.max(session.max, d.spd);
  const dist = d.odo - session.odo0, hours = (Date.now() - session.t0) / 3.6e6;
  val.sDist.textContent = dist.toFixed(1);
  val.sTime.textContent = String(Math.floor(hours)).padStart(2, '0') + ':' +
    String(Math.floor(hours * 60 % 60)).padStart(2, '0');
  val.sAvg.textContent = hours > 0.005 ? Math.round(dist / hours) : '---';
  val.sMax.textContent = Math.round(session.max);
  val.eClt.textContent = d.clt.toFixed(0);
  val.eFuel.textContent = d.fuel.toFixed(0);
  val.eRpm.textContent = Math.round(d.rpm / 10) * 10;
  val.eVolt.textContent = d.vbat.toFixed(1);
  val.eCharge.textContent = d.l.chg ? 'Генератор не заряжает' : d.rpm > 0 ? 'Заряд в норме' : 'Двигатель не запущен';
  val.tA.textContent = d.trip.toFixed(1);
  val.tOdo.textContent = Math.floor(d.odo);

  nav.update(d.spd);
  spdEl.classList.toggle('over', document.body.classList.contains('map') && d.spd > nav.limit + 5);

  for (const id in lampEls) lampEls[id].classList.toggle('on', !!d.l[id] || allLamps);
  updateAlert(allLamps ? {} : d.l);
}

function tickClock() {
  const t = new Date();
  $('clock').textContent = t.getHours() + ':' + String(t.getMinutes()).padStart(2, '0');
}
tickClock();
setInterval(tickClock, 5000);

const status = $('status');
let demoTimer = null;

function startDemo() {
  if (demoTimer) return;
  status.textContent = 'ДЕМО';
  status.setAttribute('class', 'demo');
  const t0 = performance.now();
  let odo = 23409, trip = 234.0, fuel = 31, clt = 38;
  const alertDemo = params.has('alert');
  demoTimer = setInterval(() => {
    const s = (performance.now() - t0) / 1000;
    const cycle = s % 30;
    const accel = cycle < 15 ? cycle / 15 : (30 - cycle) / 15;
    const spd = 110 * accel;
    const shiftAt = [0, 22, 42, 62, 88, 300];
    let g = 1; while (spd > shiftAt[g]) g++;
    const rpm = spd < 3 ? 850 : Math.max(900, spd / (WHEEL_M * 60 / (GEARS[g - 1] * FINAL_DRIVE * 1000)));
    odo += spd / 3600 / 20; trip += spd / 3600 / 20;
    fuel = Math.max(0, fuel - 0.004);
    clt = Math.min(89, clt + 0.06);
    const blink = Math.floor(s * 2.5) % 2 === 0;
    render({
      rpm, spd, fuel, tank: 43, clt, vbat: 14.1, odo, trip,
      l: { tl: cycle > 20 && cycle < 25 && blink, tr: false, pk: true, lb: s % 60 <= 40, hb: s % 60 > 40,
           dr: false, sb: false, fg: false, chk: s < 15, res: fuel < 6, chg: false, oil: alertDemo,
           brk: false, hot: clt >= 105, ce: false }
    });
  }, 50);
}
function stopDemo() { clearInterval(demoTimer); demoTimer = null; }

function connect() {
  if (params.has('demo') || location.protocol === 'file:') return startDemo();
  const es = new EventSource('/events');
  let gotData = false;
  const fallback = setTimeout(() => { if (!gotData) startDemo(); }, 3000);
  es.addEventListener('t', e => {
    if (!gotData) { gotData = true; clearTimeout(fallback); stopDemo(); }
    status.textContent = 'ESP32';
    status.setAttribute('class', 'live');
    render(JSON.parse(e.data));
  });
  es.onerror = () => { status.textContent = 'нет связи'; status.removeAttribute('class'); };
}
connect();

function bindTouch(target, onTap, onLongPress) {
  let timer, long = false;
  target.addEventListener('pointerdown', () => {
    long = false;
    timer = setTimeout(() => { long = true; onLongPress(); }, 1000);
  });
  target.addEventListener('pointerleave', () => clearTimeout(timer));
  target.addEventListener('pointerup', () => {
    clearTimeout(timer);
    if (!long && onTap) onTap();
  });
}
const resetStats = () => last && page === 0 && resetSession(last.odo);
bindTouch($('tapUp'), () => step(-1), resetStats);
bindTouch($('tapDown'), () => step(1), resetStats);
bindTouch($('trip'), null, () => fetch('/api/trip/reset', { method: 'POST' }).catch(() => {}));
