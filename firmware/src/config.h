#pragma once
#include <Arduino.h>

#define WIFI_SSID     "SoulDrive"
#define WIFI_PASSWORD "souldrive2109"

namespace pins {
constexpr uint8_t VBAT      = 36;
constexpr uint8_t VDPLUS    = 39;
constexpr uint8_t FUEL      = 34;
constexpr uint8_t COOLANT   = 35;

constexpr uint8_t TACHO     = 25;
constexpr uint8_t SPEED     = 26;

constexpr uint8_t TURN_L    = 27;
constexpr uint8_t TURN_R    = 14;
constexpr uint8_t HIGH_BEAM = 13;
constexpr uint8_t LOW_BEAM  = 23;
constexpr uint8_t PARKING   = 4;
constexpr uint8_t REAR_FOG  = 16;
constexpr uint8_t OIL       = 17;
constexpr uint8_t BRAKE     = 18;
constexpr uint8_t CHOKE     = 19;
constexpr uint8_t DOOR      = 32;
constexpr uint8_t SEATBELT  = 33;
}

constexpr float DIVIDER_RATIO   = (100.0f + 22.0f) / 22.0f;
constexpr float SENDER_VCC_MV   = 3300.0f;
constexpr float FUEL_PULLUP_OHM = 100.0f;
constexpr float CLT_PULLUP_OHM  = 1000.0f;

constexpr float    SPARKS_PER_REV    = 2.0f;
constexpr uint32_t TACHO_MIN_US      = 2500;
constexpr uint32_t TACHO_TIMEOUT_US  = 500000;
constexpr uint32_t GLITCH_PERCENT    = 60;
constexpr uint16_t RPM_REDLINE       = 6000;

constexpr float    SPEED_PULSES_PER_KM = 6004.0f;
constexpr uint32_t SPEED_MIN_US        = 1000;
constexpr uint32_t SPEED_TIMEOUT_US    = 1500000;
constexpr float    SPEED_MAX_KMH_PER_S = 40.0f;

struct CalPoint { float ohm; float value; };

constexpr float FUEL_TANK_L = 43.0f;
constexpr float FUEL_RESERVE_L = 6.0f;
constexpr CalPoint FUEL_TABLE[] = {
  {315.0f,  0.0f},
  {250.0f,  5.0f},
  {200.0f, 10.0f},
  {118.0f, 21.5f},
  { 60.0f, 32.0f},
  {  7.0f, 43.0f},
};

constexpr float CLT_WARN_C = 105.0f;
constexpr CalPoint CLT_TABLE[] = {
  {700.0f,  30.0f},
  {400.0f,  40.0f},
  {210.0f,  60.0f},
  {128.0f,  80.0f},
  {100.0f,  90.0f},
  { 80.0f, 100.0f},
  { 63.0f, 110.0f},
  { 50.0f, 120.0f},
};

constexpr float VBAT_LOW_V        = 11.8f;
constexpr float VBAT_HIGH_V       = 14.8f;
constexpr float CHARGE_FAULT_DV   = 3.0f;

constexpr bool SEATBELT_LOW_WHEN_UNBUCKLED = true;

constexpr float SENDER_SHORT_MV  = 155.0f;
constexpr float SENDER_OPEN_MV   = 2900.0f;

constexpr uint32_t TELEMETRY_PERIOD_MS = 50;
constexpr uint32_t DEBOUNCE_MS         = 20;
constexpr uint32_t ODO_SAVE_METERS     = 100;
