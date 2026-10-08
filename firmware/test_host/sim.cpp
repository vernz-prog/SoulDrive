#include <algorithm>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "Arduino.h"

#include "../src/inputs.cpp"
#include "../src/telemetry_json.h"

namespace sim {
uint64_t nowUs = 0;
int pinLevel[40];
float pinMv[40];
void (*isr[40])() = {};
std::mt19937 rng(2109);

float adcReadMv(uint8_t pin) {
  std::normal_distribution<float> noise(0.0f, 5.0f);
  float v = pinMv[pin];
  if (v > 2450) v -= (v - 2450) * 0.02f;
  v = std::min(std::max(v, 142.0f), 3150.0f) + noise(rng);
  return std::max(v, 0.0f);
}
}

constexpr double TRUE_PULSES_PER_KM = 6004.0;

struct Car {
  double rpm = 0, kmh = 0;
  double vbat = 12.4, vdplus = 0.8;
  double fuelL = 35, coolantC = 35;
  double fuelOhmOverride = -1, cltOhmOverride = -1;
  bool coilRinging = true;
  double emiPerSec = 0;
  bool lamp[40] = {};
} car;

template <size_t N>
double tableOhm(const CalPoint (&t)[N], double value) {
  if (value <= t[0].value) return t[0].ohm;
  if (value >= t[N - 1].value) return t[N - 1].ohm;
  for (size_t i = 1; i < N; i++)
    if (value <= t[i].value)
      return t[i - 1].ohm + (t[i].ohm - t[i - 1].ohm) * (value - t[i - 1].value) / (t[i].value - t[i - 1].value);
  return t[N - 1].ohm;
}

double senderMv(double ohm, double pullup) {
  if (std::isinf(ohm)) return SENDER_VCC_MV;
  return SENDER_VCC_MV * ohm / (ohm + pullup);
}

void applyAnalog() {
  double fuelOhm = car.fuelOhmOverride >= 0 ? car.fuelOhmOverride : tableOhm(FUEL_TABLE, car.fuelL);
  double cltOhm = car.cltOhmOverride >= 0 ? car.cltOhmOverride : tableOhm(CLT_TABLE, car.coolantC);
  sim::pinMv[pins::FUEL] = senderMv(fuelOhm, FUEL_PULLUP_OHM);
  sim::pinMv[pins::COOLANT] = senderMv(cltOhm, CLT_PULLUP_OHM);
  sim::pinMv[pins::VBAT] = car.vbat * 1000 / DIVIDER_RATIO;
  sim::pinMv[pins::VDPLUS] = car.vdplus * 1000 / DIVIDER_RATIO;
}

struct Event { uint64_t t; uint8_t pin; int level; bool edge; };
std::vector<Event> queue;
uint64_t nextSpark = 0, nextSpeed = 0, nextEmi = 0;
const uint8_t LAMP_PINS[] = {pins::TURN_L, pins::TURN_R, pins::HIGH_BEAM, pins::LOW_BEAM, pins::PARKING,
                             pins::REAR_FOG, pins::OIL, pins::BRAKE, pins::CHOKE, pins::DOOR, pins::SEATBELT};
bool lampShown[40] = {};

void push(uint64_t t, uint8_t pin, int level, bool edge) { queue.push_back({t, pin, level, edge}); }

void pulse(uint64_t t, uint8_t pin) {
  push(t, pin, LOW, true);
  push(t + 80, pin, HIGH, false);
}

void schedule(uint64_t until) {
  uint64_t now = sim::nowUs;
  if (car.rpm > 0) {
    double period = 60e6 / (car.rpm * SPARKS_PER_REV);
    if (nextSpark < now) nextSpark = now + (uint64_t)period;
    while (nextSpark <= until) {
      pulse(nextSpark, pins::TACHO);
      if (car.coilRinging) { pulse(nextSpark + 120, pins::TACHO); pulse(nextSpark + 650, pins::TACHO); }
      nextSpark += (uint64_t)period;
    }
  } else {
    nextSpark = 0;
  }
  if (car.kmh > 0.05) {
    double period = 3600e6 / (car.kmh * TRUE_PULSES_PER_KM);
    if (nextSpeed < now) nextSpeed = now + (uint64_t)period;
    while (nextSpeed <= until) { pulse(nextSpeed, pins::SPEED); nextSpeed += (uint64_t)period; }
  } else {
    nextSpeed = 0;
  }
  if (car.emiPerSec > 0) {
    std::exponential_distribution<double> gap(car.emiPerSec / 1e6);
    if (nextEmi < now) nextEmi = now + (uint64_t)gap(sim::rng);
    while (nextEmi <= until) { pulse(nextEmi, pins::SPEED); nextEmi += (uint64_t)gap(sim::rng); }
  }
  for (uint8_t p : LAMP_PINS) {
    if (car.lamp[p] == lampShown[p]) continue;
    lampShown[p] = car.lamp[p];
    int on = car.lamp[p] ? LOW : HIGH, off = car.lamp[p] ? HIGH : LOW;
    for (int k = 0; k < 3; k++) { push(now + k * 1000, p, on, false); push(now + k * 1000 + 400, p, off, false); }
    push(now + 3000, p, on, false);
  }
  std::sort(queue.begin(), queue.end(), [](const Event &a, const Event &b) { return a.t < b.t; });
}

void advanceTo(uint64_t t) {
  size_t i = 0;
  for (; i < queue.size() && queue[i].t <= t; i++) {
    sim::nowUs = queue[i].t;
    bool wasHigh = sim::pinLevel[queue[i].pin] == HIGH;
    sim::pinLevel[queue[i].pin] = queue[i].level;
    if (queue[i].edge && wasHigh && sim::isr[queue[i].pin]) sim::isr[queue[i].pin]();
  }
  queue.erase(queue.begin(), queue.begin() + i);
  sim::nowUs = t;
}

Telemetry tel{};
struct Frame { double t; Telemetry tel; std::string json; };
std::vector<Frame> frames;
FILE *stream = nullptr;
double streamFrom = 1e18;
uint64_t t0;
uint32_t lastSendMs = 0;
std::function<void()> perLoop;

double simSec() { return (sim::nowUs - t0) / 1e6; }

void run(double seconds) {
  uint64_t end = sim::nowUs + (uint64_t)(seconds * 1e6);
  while (sim::nowUs < end) {
    applyAnalog();
    uint64_t next = sim::nowUs + 1000;
    schedule(next);
    inputs::update(tel);
    if (perLoop) perLoop();
    if (millis() - lastSendMs >= TELEMETRY_PERIOD_MS) {
      lastSendMs = millis();
      char buf[512];
      toJson(tel, buf, sizeof buf);
      frames.push_back({simSec(), tel, buf});
      if (stream && simSec() >= streamFrom) fprintf(stream, "%.3f %s\n", simSec(), buf);
    }
    advanceTo(next);
  }
}

int passed = 0, failed = 0;
std::vector<std::string> warnings;
void check(const std::string &name, bool ok, const std::string &detail) {
  printf("  %s  %-58s %s\n", ok ? "OK  " : "FAIL", name.c_str(), detail.c_str());
  (ok ? passed : failed)++;
}
void warn(const std::string &s) { warnings.push_back(s); printf("  WARN  %s\n", s.c_str()); }
std::string fmt(const char *f, double a, double b = 0, double c = 0) {
  char s[160]; snprintf(s, sizeof s, f, a, b, c); return s;
}
template <class F> std::pair<double, double> span(double sec, F get) {
  double lo = 1e18, hi = -1e18, from = simSec() - sec;
  for (auto it = frames.rbegin(); it != frames.rend() && it->t >= from; ++it) {
    double v = get(it->tel); lo = std::min(lo, v); hi = std::max(hi, v);
  }
  return {lo, hi};
}
template <class F> bool everIn(double from, double to, F pred) {
  for (auto &f : frames) if (f.t >= from && f.t <= to && pred(f.tel)) return true;
  return false;
}
void section(const char *s) { printf("\n%s\n", s); }

int main(int argc, char **argv) {
  std::string mode = argc > 1 ? argv[1] : "all";
  for (int &l : sim::pinLevel) l = HIGH;
  sim::nowUs = (1ULL << 32) - 4'500'000ULL;
  t0 = sim::nowUs;
  if (const char *p = getenv("SIM_STREAM")) { stream = fopen(p, "w"); streamFrom = 0; }

  if (mode == "resume") {
    inputs::begin();
    run(0.5);
    printf("%.3f %.3f\n", tel.odoKm, tel.tripKm);
    return 0;
  }

  inputs::begin();
  car.lamp[pins::OIL] = true;
  car.lamp[pins::BRAKE] = true;
  car.lamp[pins::SEATBELT] = true;

  section("1. Включение зажигания (двигатель стоит)");
  run(0.6);
  check("нет ложной лампы резерва топлива сразу после включения",
        !everIn(0.15, 0.6, [](auto &t) { return t.lamps.fuelReserve; }),
        fmt("топливо через 0,5 с: %.1f л (в баке 35 л)", frames.back().tel.fuelL));
  run(2.4);
  check("обороты 0, скорость 0", tel.rpm == 0 && tel.speedKmh == 0, fmt("%.0f об/мин, %.1f км/ч", tel.rpm, tel.speedKmh));
  check("лампа заряда горит (генератор не крутится)", tel.lamps.charge, fmt("Vbat %.2f, D+ %.2f", tel.vbat, tel.vdplus));
  check("лампа давления масла горит", tel.lamps.oil, "");
  check("напряжение 12,4 В ±0,2", std::fabs(tel.vbat - 12.4) < 0.2, fmt("%.2f В", tel.vbat));
  check("топливо 35 л ±1 (успокоилось за 3 с)", std::fabs(tel.fuelL - 35) < 1, fmt("%.1f л", tel.fuelL));
  check("Check Engine не горит", !tel.lamps.checkEngine, "");

  section("2. Пуск, холостой ход 850 об/мин (звон катушки включён)");
  car.rpm = 200; car.vbat = 10.5; run(1.0);
  auto crank = span(0.4, [](auto &t) { return (double)t.rpm; });
  check("прокрутка стартером 200 об/мин видна", crank.first > 180 && crank.second < 220, fmt("%.0f…%.0f об/мин", crank.first, crank.second));
  car.rpm = 850; double tCatch = simSec(); run(0.6);
  double caught = 1e9;
  for (auto &f : frames) if (f.t > tCatch && std::fabs(f.tel.rpm - 850) < 20) { caught = f.t - tCatch; break; }
  check("двигатель схватил: 850 об/мин видно не позже 0,3 с", caught <= 0.3, fmt("через %.2f с", caught));
  car.rpm = 850; car.vbat = 14.2; car.vdplus = 14.2; car.lamp[pins::OIL] = false;
  run(3);
  auto r = span(1, [](auto &t) { return (double)t.rpm; });
  check("обороты 850 ±2 %, звон катушки отфильтрован", r.first > 833 && r.second < 867, fmt("%.0f…%.0f об/мин", r.first, r.second));
  check("лампа заряда погасла", !tel.lamps.charge, "");
  check("лампа масла погасла", !tel.lamps.oil, "");
  check("напряжение 14,2 В ±0,3", std::fabs(tel.vbat - 14.2) < 0.3, fmt("%.2f В", tel.vbat));
  check("переполнение micros() пройдено без сбоев", (uint32_t)sim::nowUs < (uint32_t)t0, fmt("время платы %.0f мкс", (double)(uint32_t)sim::nowUs));

  section("3. Обороты по всей шкале");
  for (double rpm : {1500.0, 3000.0, 4500.0, 6000.0, 6500.0}) {
    car.rpm = rpm;
    run(1.2);
    auto s = span(0.5, [](auto &t) { return (double)t.rpm; });
    double err = std::max(std::fabs(s.first - rpm), std::fabs(s.second - rpm)) / rpm * 100;
    check(fmt("%.0f об/мин ±2 %%", rpm), err < 2, fmt("%.0f…%.0f (ошибка %.2f %%)", s.first, s.second, err));
  }
  car.rpm = 1000; run(1);
  double worst = 0;
  for (int i = 0; i <= 50; i++) {
    car.rpm = 1000 + 80 * i; run(0.01);
    if (i > 5) worst = std::max(worst, (car.rpm - tel.rpm) / 8000.0);
  }
  run(0.2);
  check("перегазовка 1000→5000 за 0,5 с: отставание ≤ 0,1 с, не залипает", worst <= 0.1 && std::fabs(tel.rpm - 5000) < 50,
        fmt("отставание %.0f мс, в конце %.0f об/мин", worst * 1000, tel.rpm));
  car.rpm = 2500;

  section("4. Скорость (датчик ВАЗ-2110, 6004 имп/км)");
  car.lamp[pins::BRAKE] = false; car.lamp[pins::SEATBELT] = false;
  for (double v : {3.0, 5.0, 20.0, 60.0, 120.0, 180.0}) {
    car.kmh = v;
    run(3);
    auto s = span(0.5, [](auto &t) { return (double)t.speedKmh; });
    double err = std::max(std::fabs(s.first - v), std::fabs(s.second - v));
    check(fmt("%.0f км/ч ±1 %%", v), err <= std::max(0.01 * v, 0.1), fmt("%.2f…%.2f км/ч", s.first, s.second));
  }

  section("5. Помехи на линии датчика скорости (5 выбросов/с, 60 км/ч)");
  car.kmh = 60; run(4);
  car.emiPerSec = 5; run(10);
  auto e = span(10, [](auto &t) { return (double)t.speedKmh; });
  double emiErr = std::max(60 - e.first, e.second - 60);
  check("помехи не дают скачков скорости больше ±3 км/ч", emiErr <= 3, fmt("разброс %.1f…%.1f км/ч", e.first, e.second));
  double odoA = tel.odoKm; run(60);
  double dEmi = tel.odoKm - odoA;
  check("5 помех/с за минуту не накручивают пробег (1,000 км ±0,5 %)", std::fabs(dEmi - 1.0) < 0.005, fmt("%.3f км", dEmi));
  odoA = tel.odoKm; car.emiPerSec = 50; run(60);
  dEmi = tel.odoKm - odoA;
  auto e50 = span(60, [](auto &t) { return (double)t.speedKmh; });
  printf("  INFO  стресс 50 помех/с: пробег %.3f км вместо 1,000, скорость %.1f…%.1f км/ч\n", dEmi, e50.first, e50.second);
  car.emiPerSec = 0; run(3);

  section("6. Пробег: 10 км с переменной скоростью, сброс суточного");
  inputs::resetTrip();
  run(0.1);
  double odo0 = tel.odoKm, dist = 0;
  int writes0 = prefs.writes;
  perLoop = [&] { dist += car.kmh / 3600.0 / 1000.0; };
  for (int i = 0; dist < 10.0; i++) { car.kmh = 70 + 30 * std::sin(i * 0.37); car.rpm = car.kmh * 40; run(1); }
  perLoop = nullptr;
  double dOdo = tel.odoKm - odo0;
  check("одометр: 10 км ±0,1 %", std::fabs(dOdo - dist) / dist < 0.001, fmt("модель %.3f км, панель %.3f км", dist, dOdo));
  check("суточный пробег после сброса совпадает", std::fabs(tel.tripKm - dOdo) < 0.002, fmt("%.3f км", tel.tripKm));
  int wr = prefs.writes - writes0;
  check("запись во флеш раз в 100 м (не чаще)", wr <= 2 * 101, fmt("%.0f записей за %.1f км", wr, dist));

  section("7. Датчик уровня топлива");
  car.kmh = 0; car.rpm = 850; run(2);
  for (double l : {43.0, 32.0, 21.5, 10.0, 5.0, 0.0}) {
    car.fuelL = l;
    run(30);
    double ohm = tableOhm(FUEL_TABLE, l), mv = senderMv(ohm, FUEL_PULLUP_OHM);
    check(fmt("%.1f л ±1 л", l), std::fabs(tel.fuelL - l) < 1,
          fmt("%.1f л, датчик %.0f Ом, на АЦП %.0f мВ", tel.fuelL, ohm, mv));
    if (l == 43) check("полный бак: нет ложного Check Engine", !tel.lamps.checkEngine, "");
    if (l <= 5) check(fmt("лампа резерва при %.0f л", l), tel.lamps.fuelReserve, "");
    if (l >= 10) check(fmt("нет лампы резерва при %.0f л", l), !tel.lamps.fuelReserve, "");
  }
  car.fuelL = 30; run(30);

  section("8. Датчик температуры ОЖ ТМ-106");
  for (double c : {40.0, 60.0, 90.0, 100.0, 106.0, 115.0}) {
    car.coolantC = c;
    run(6);
    double ohm = tableOhm(CLT_TABLE, c);
    check(fmt("%.0f °C ±2 °C", c), std::fabs(tel.coolantC - c) < 2,
          fmt("%.1f °C, датчик %.0f Ом, на АЦП %.0f мВ", tel.coolantC, ohm, senderMv(ohm, CLT_PULLUP_OHM)));
    check(std::string(c >= CLT_WARN_C ? "лампа перегрева горит" : "лампа перегрева не горит") + fmt(" при %.0f °C", c),
          tel.lamps.overheat == (c >= CLT_WARN_C), "");
  }
  car.coolantC = 88; run(6);

  section("9. Неисправности проводки (самодиагностика → Check Engine)");
  car.fuelOhmOverride = INFINITY; run(1);
  check("обрыв провода датчика топлива", tel.lamps.checkEngine, "");
  car.fuelOhmOverride = -1; run(1);
  check("после восстановления лампа гаснет", !tel.lamps.checkEngine, "");
  car.cltOhmOverride = INFINITY; run(1);
  check("обрыв провода ТМ-106", tel.lamps.checkEngine, "");
  car.cltOhmOverride = 0.3; run(1);
  bool cltShort = tel.lamps.checkEngine;
  check("замыкание провода ТМ-106 на массу", cltShort, fmt("панель видит %.0f °C", tel.coolantC));
  car.fuelOhmOverride = 0.3; car.cltOhmOverride = -1; run(1);
  check("замыкание провода датчика топлива на массу", tel.lamps.checkEngine, fmt("панель видит %.1f л", tel.fuelL));
  car.fuelOhmOverride = -1; run(30);

  section("10. Бортсеть");
  car.vdplus = 0.3; run(1);
  check("обрыв ремня генератора: лампа заряда", tel.lamps.charge, "");
  car.vbat = 11.5; run(1);
  check("недозаряд 11,5 В на ходу: Check Engine", tel.lamps.checkEngine, fmt("%.2f В", tel.vbat));
  car.vbat = 15.2; car.vdplus = 15.2; run(1);
  check("перезаряд 15,2 В: Check Engine", tel.lamps.checkEngine, fmt("%.2f В", tel.vbat));
  check("перезаряд 15,2 В: напряжение измерено ±0,4 В", std::fabs(tel.vbat - 15.2) < 0.4, fmt("%.2f В", tel.vbat));
  car.vbat = 14.2; car.vdplus = 14.2; run(1);
  check("норма 14,2 В: Check Engine погас", !tel.lamps.checkEngine, "");

  section("11. Контрольные лампы (оптопары, дребезг контактов)");
  struct L { uint8_t pin; const char *name; bool Lamps::*f; };
  const L lamps[] = {{pins::TURN_L, "левый поворот", &Lamps::turnL}, {pins::TURN_R, "правый поворот", &Lamps::turnR},
    {pins::HIGH_BEAM, "дальний", &Lamps::highBeam}, {pins::LOW_BEAM, "ближний", &Lamps::lowBeam},
    {pins::PARKING, "габариты", &Lamps::parking}, {pins::REAR_FOG, "задняя ПТФ", &Lamps::rearFog},
    {pins::OIL, "давление масла", &Lamps::oil}, {pins::BRAKE, "ручник / ТЖ", &Lamps::brake},
    {pins::CHOKE, "подсос", &Lamps::choke}, {pins::DOOR, "дверь", &Lamps::door},
    {pins::SEATBELT, "ремень", &Lamps::seatbelt}};
  for (auto &a : lamps) {
    car.lamp[a.pin] = true; run(0.1);
    bool only = tel.lamps.*a.f;
    for (auto &b : lamps) if (&b != &a && tel.lamps.*b.f) only = false;
    car.lamp[a.pin] = false; run(0.1);
    check(std::string(a.name) + ": горит только своя лампа и гаснет", only && !(tel.lamps.*a.f), "");
  }
  int flashes = 0; bool prev = false;
  perLoop = [&] { if (tel.lamps.turnL && !prev) flashes++; prev = tel.lamps.turnL; };
  for (int i = 0; i < 15; i++) {
    car.lamp[pins::TURN_L] = true; run(1.0 / 3);
    car.lamp[pins::TURN_L] = false; run(1.0 / 3);
  }
  perLoop = nullptr;
  check("поворотник 90 раз/мин: 15 вспышек за 10 с, дребезг отфильтрован", flashes == 15, fmt("%.0f вспышек", flashes));

  section("12. Остановка");
  car.kmh = 40; run(2);
  car.kmh = 0; double tStop = simSec(); run(2);
  double spdZero = 1e9;
  for (auto &f : frames) if (f.t > tStop && f.tel.speedKmh == 0) { spdZero = f.t - tStop; break; }
  check("скорость падает в 0 не позже 1,6 с после остановки колёс", spdZero <= 1.6, fmt("через %.2f с", spdZero));
  car.rpm = 0; car.vdplus = 0.8; car.lamp[pins::OIL] = true; tStop = simSec(); run(1.5);
  double rpmZero = 1e9;
  for (auto &f : frames) if (f.t > tStop && f.tel.rpm == 0) { rpmZero = f.t - tStop; break; }
  check("обороты падают в 0 не позже 0,6 с после глушения", rpmZero <= 0.6, fmt("через %.2f с", rpmZero));
  check("снова горят лампы заряда и масла", tel.lamps.charge && tel.lamps.oil, "");

  section("13. JSON для планшета");
  size_t maxLen = 0;
  for (auto &f : frames) maxLen = std::max(maxLen, f.json.size());
  check("кадр помещается в буфер 512 байт", maxLen < 512, fmt("самый длинный кадр %.0f байт", maxLen));
  check(fmt("кадров отправлено: %.0f за %.0f с (20 в секунду)", frames.size(), simSec()),
        std::fabs(frames.size() / simSec() - 20) < 0.5, "");
  if (FILE *f = fopen("frames.jsonl", "w")) {
    for (auto &fr : frames) fprintf(f, "%s\n", fr.json.c_str());
    fclose(f);
  }

  printf("\nИтого: %d проверок пройдено, %d не пройдено, предупреждений: %zu\n", passed, failed, warnings.size());
  printf("odo %.3f\n", tel.odoKm);
  if (stream) fclose(stream);
  return failed ? 1 : 0;
}
