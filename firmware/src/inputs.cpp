#include "inputs.h"

#include <Preferences.h>

#include "config.h"

namespace {

constexpr uint8_t NPER = 5;

struct PulseInput {
  volatile uint32_t lastUs = 0;
  volatile uint32_t countedUs = 0;
  volatile uint32_t periods[NPER] = {};
  volatile uint8_t n = 0, idx = 0;
  volatile uint32_t count = 0;
  const uint32_t minUs, timeoutUs;
  PulseInput(uint32_t minUs, uint32_t timeoutUs) : minUs(minUs), timeoutUs(timeoutUs) {}
};

PulseInput tacho(TACHO_MIN_US, TACHO_TIMEOUT_US);
PulseInput speed(SPEED_MIN_US, SPEED_TIMEOUT_US);
portMUX_TYPE pulseMux = portMUX_INITIALIZER_UNLOCKED;

uint32_t IRAM_ATTR median(const PulseInput &p) {
  uint32_t v[NPER];
  uint8_t n = p.n;
  for (uint8_t i = 0; i < n; i++) {
    uint32_t x = p.periods[i];
    uint8_t j = i;
    for (; j > 0 && v[j - 1] > x; j--) v[j] = v[j - 1];
    v[j] = x;
  }
  return n ? v[n / 2] : 0;
}

void IRAM_ATTR onPulse(PulseInput &p) {
  uint32_t now = micros();
  uint32_t dt = now - p.lastUs;
  if (dt < p.minUs) return;
  portENTER_CRITICAL_ISR(&pulseMux);
  if (dt > p.timeoutUs) {
    p.n = 0;
  } else {
    p.periods[p.idx] = dt;
    p.idx = (p.idx + 1) % NPER;
    if (p.n < NPER) p.n++;
  }
  p.lastUs = now;
  uint32_t med = median(p);
  if (med == 0 || (uint64_t)(now - p.countedUs) * 100 >= (uint64_t)med * GLITCH_PERCENT) {
    p.count++;
    p.countedUs = now;
  }
  portEXIT_CRITICAL_ISR(&pulseMux);
}

void IRAM_ATTR onTacho() { onPulse(tacho); }
void IRAM_ATTR onSpeed() { onPulse(speed); }

float pulseHz(PulseInput &p) {
  portENTER_CRITICAL(&pulseMux);
  uint32_t period = median(p);
  uint32_t last = p.lastUs;
  portEXIT_CRITICAL(&pulseMux);
  if (period == 0 || micros() - last > p.timeoutUs) return 0.0f;
  return 1e6f / period;
}

uint32_t pulseCount(PulseInput &p) {
  portENTER_CRITICAL(&pulseMux);
  uint32_t c = p.count;
  portEXIT_CRITICAL(&pulseMux);
  return c;
}

struct DigitalInput {
  const uint8_t pin;
  bool state = false;
  bool raw = false;
  uint32_t changedMs = 0;
  explicit DigitalInput(uint8_t pin) : pin(pin) {}

  bool read() {
    bool now = digitalRead(pin) == LOW;
    if (now != raw) {
      raw = now;
      changedMs = millis();
    } else if (now != state && millis() - changedMs >= DEBOUNCE_MS) {
      state = now;
    }
    return state;
  }
};

DigitalInput dTurnL(pins::TURN_L), dTurnR(pins::TURN_R), dHigh(pins::HIGH_BEAM),
    dLow(pins::LOW_BEAM), dPark(pins::PARKING), dFog(pins::REAR_FOG), dOil(pins::OIL),
    dBrake(pins::BRAKE), dChoke(pins::CHOKE), dDoor(pins::DOOR), dBelt(pins::SEATBELT);

float readMv(uint8_t pin) {
  uint32_t sum = 0;
  for (int i = 0; i < 16; i++) sum += analogReadMilliVolts(pin);
  return sum / 16.0f;
}

struct Sender { float ohm; bool fault; };
Sender readSender(uint8_t pin, float pullupOhm) {
  float mv = readMv(pin);
  if (mv > SENDER_OPEN_MV) return {INFINITY, true};
  return {pullupOhm * mv / (SENDER_VCC_MV - mv), mv < SENDER_SHORT_MV};
}

template <size_t N>
float interpolate(const CalPoint (&table)[N], float ohm) {
  if (ohm >= table[0].ohm) return table[0].value;
  if (ohm <= table[N - 1].ohm) return table[N - 1].value;
  for (size_t i = 1; i < N; i++) {
    if (ohm >= table[i].ohm) {
      const CalPoint &a = table[i - 1], &b = table[i];
      return a.value + (b.value - a.value) * (a.ohm - ohm) / (a.ohm - b.ohm);
    }
  }
  return table[N - 1].value;
}

float smooth(float prev, float now, float k) { return prev + (now - prev) * k; }

Preferences prefs;
double odoBaseKm = 0;
double tripBaseKm = 0;
uint32_t lastSavedMeters = 0;
volatile bool tripResetRequested = false;

}

namespace inputs {

void begin() {
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  for (auto *d : {&dTurnL, &dTurnR, &dHigh, &dLow, &dPark, &dFog, &dOil, &dBrake, &dChoke, &dDoor, &dBelt})
    pinMode(d->pin, INPUT_PULLUP);

  pinMode(pins::TACHO, INPUT_PULLUP);
  pinMode(pins::SPEED, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(pins::TACHO), onTacho, FALLING);
  attachInterrupt(digitalPinToInterrupt(pins::SPEED), onSpeed, FALLING);

  prefs.begin("souldrive", false);
  odoBaseKm = prefs.getDouble("odo", 0.0);
  tripBaseKm = prefs.getDouble("trip", 0.0);
}

void resetTrip() { tripResetRequested = true; }

void update(Telemetry &t) {
#if SOULDRIVE_DEMO
  float s = millis() / 1000.0f;
  float cycle = fmodf(s, 30.0f);
  float accel = cycle < 15 ? cycle / 15.0f : (30.0f - cycle) / 15.0f;
  t.speedKmh = 110.0f * accel;
  t.rpm = 850 + (uint16_t)(fmodf(t.speedKmh, 30.0f) / 30.0f * 3500.0f) + (uint16_t)(accel * 800);
  t.fuelL = 30.0f - fmodf(s / 60.0f, 25.0f);
  t.coolantC = min(90.0f, 40.0f + s / 2.0f);
  t.vbat = t.rpm > 0 ? 14.1f : 12.5f;
  t.vdplus = t.vbat;
  t.odoKm = 128734.0 + s * t.speedKmh / 3600.0;
  t.tripKm = s * 0.015;
  bool blink = ((int)(s * 2.5f)) % 2;
  t.lamps = {};
  t.lamps.turnL = cycle > 20 && cycle < 25 && blink;
  t.lamps.parking = true;
  t.lamps.lowBeam = true;
  t.lamps.choke = s < 20;
  t.lamps.fuelReserve = t.fuelL < FUEL_RESERVE_L;
  return;
#endif

  float rpm = pulseHz(tacho) * 60.0f / SPARKS_PER_REV;
  t.rpm = rpm == 0 ? 0 : (uint16_t)smooth(t.rpm, rpm, 0.3f);

  float kmh = pulseHz(speed) * 3600.0f / SPEED_PULSES_PER_KM;
  static uint32_t lastSpeedUs = micros();
  float step = (micros() - lastSpeedUs) / 1e6f * SPEED_MAX_KMH_PER_S;
  lastSpeedUs = micros();
  float target = smooth(t.speedKmh, kmh, 0.3f);
  t.speedKmh = kmh == 0 ? 0 : constrain(target, t.speedKmh - step, t.speedKmh + step);

  static uint32_t lastAnalogMs = 0;
  static bool senderFault = false, firstAnalog = true;
  if (firstAnalog || millis() - lastAnalogMs >= 100) {
    lastAnalogMs = millis();
    Sender fuel = readSender(pins::FUEL, FUEL_PULLUP_OHM);
    Sender clt = readSender(pins::COOLANT, CLT_PULLUP_OHM);
    senderFault = fuel.fault || clt.fault;
    float k = firstAnalog ? 1.0f : 0.3f;
    t.vbat = smooth(t.vbat, readMv(pins::VBAT) * DIVIDER_RATIO / 1000.0f, k);
    t.vdplus = smooth(t.vdplus, readMv(pins::VDPLUS) * DIVIDER_RATIO / 1000.0f, k);
    t.fuelL = smooth(t.fuelL, interpolate(FUEL_TABLE, fuel.ohm), firstAnalog ? 1.0f : 0.02f);
    t.coolantC = smooth(t.coolantC, interpolate(CLT_TABLE, clt.ohm), firstAnalog ? 1.0f : 0.1f);
    firstAnalog = false;
  }

  double km = pulseCount(speed) / SPEED_PULSES_PER_KM;
  if (tripResetRequested) {
    tripResetRequested = false;
    tripBaseKm = -km;
    prefs.putDouble("trip", 0.0);
  }
  t.odoKm = odoBaseKm + km;
  t.tripKm = tripBaseKm + km;
  uint32_t meters = (uint32_t)(km * 1000.0);
  if (meters - lastSavedMeters >= ODO_SAVE_METERS) {
    lastSavedMeters = meters;
    prefs.putDouble("odo", t.odoKm);
    prefs.putDouble("trip", t.tripKm);
  }

  Lamps &l = t.lamps;
  l.turnL = dTurnL.read();
  l.turnR = dTurnR.read();
  l.highBeam = dHigh.read();
  l.lowBeam = dLow.read();
  l.parking = dPark.read();
  l.rearFog = dFog.read();
  l.oil = dOil.read();
  l.brake = dBrake.read();
  l.choke = dChoke.read();
  l.door = dDoor.read();
  l.seatbelt = dBelt.read() == SEATBELT_LOW_WHEN_UNBUCKLED;
  l.charge = t.vbat - t.vdplus > CHARGE_FAULT_DV;
  l.fuelReserve = t.fuelL < FUEL_RESERVE_L;
  l.overheat = t.coolantC >= CLT_WARN_C;
  bool voltageFault = t.rpm > 0 && (t.vbat < VBAT_LOW_V || t.vbat > VBAT_HIGH_V);
  l.checkEngine = senderFault || voltageFault;
}

}
