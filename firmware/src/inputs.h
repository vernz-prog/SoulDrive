#pragma once
#include <Arduino.h>

struct Lamps {
  bool turnL, turnR, highBeam, lowBeam, parking, rearFog;
  bool oil, brake, choke, door, seatbelt;
  bool charge, fuelReserve, overheat, checkEngine;
};

struct Telemetry {
  uint16_t rpm;
  float    speedKmh;
  float    fuelL;
  float    coolantC;
  float    vbat;
  float    vdplus;
  double   odoKm;
  double   tripKm;
  Lamps    lamps;
};

namespace inputs {
void begin();
void update(Telemetry &t);
void resetTrip();
}
