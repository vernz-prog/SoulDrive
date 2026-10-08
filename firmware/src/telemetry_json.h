#pragma once
#include <stdio.h>

#include "config.h"
#include "inputs.h"

inline size_t toJson(const Telemetry &t, char *buf, size_t len) {
  const Lamps &l = t.lamps;
  return snprintf(buf, len,
                  "{\"rpm\":%u,\"spd\":%.1f,\"fuel\":%.1f,\"tank\":%.0f,\"clt\":%.1f,"
                  "\"vbat\":%.2f,\"odo\":%.1f,\"trip\":%.1f,\"redline\":%u,"
                  "\"l\":{\"tl\":%d,\"tr\":%d,\"hb\":%d,\"lb\":%d,\"pk\":%d,\"fg\":%d,\"oil\":%d,"
                  "\"brk\":%d,\"chk\":%d,\"dr\":%d,\"sb\":%d,\"chg\":%d,\"res\":%d,\"hot\":%d,\"ce\":%d}}",
                  t.rpm, t.speedKmh, t.fuelL, FUEL_TANK_L, t.coolantC, t.vbat, t.odoKm,
                  t.tripKm, RPM_REDLINE, l.turnL, l.turnR, l.highBeam, l.lowBeam, l.parking,
                  l.rearFog, l.oil, l.brake, l.choke, l.door, l.seatbelt, l.charge, l.fuelReserve, l.overheat,
                  l.checkEngine);
}
