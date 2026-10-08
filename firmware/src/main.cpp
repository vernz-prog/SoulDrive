#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WiFi.h>

#include "config.h"
#include "inputs.h"
#include "telemetry_json.h"

#ifndef SOULDRIVE_DEMO
#define SOULDRIVE_DEMO 0
#endif

static AsyncWebServer server(80);
static AsyncEventSource events("/events");
static Telemetry telemetry{};

void setup() {
  Serial.begin(115200);
  Serial.println(SOULDRIVE_DEMO ? "SoulDrive (DEMO)" : "SoulDrive");

  inputs::begin();

  if (!LittleFS.begin(true)) Serial.println("LittleFS: ошибка, выполните pio run -t uploadfs");

  WiFi.mode(WIFI_AP);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  MDNS.begin("souldrive");
  Serial.printf("Wi-Fi \"%s\", откройте http://%s\n", WIFI_SSID,
                WiFi.softAPIP().toString().c_str());

  server.addHandler(&events);
  server.on("/api/state", HTTP_GET, [](AsyncWebServerRequest *req) {
    char buf[512];
    toJson(telemetry, buf, sizeof buf);
    req->send(200, "application/json", buf);
  });
  server.on("/api/trip/reset", HTTP_POST, [](AsyncWebServerRequest *req) {
    inputs::resetTrip();
    req->send(204);
  });
  server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");
  server.onNotFound([](AsyncWebServerRequest *req) { req->send(404, "text/plain", "Not found"); });
  server.begin();
}

void loop() {
  inputs::update(telemetry);

  static uint32_t lastSendMs = 0;
  if (millis() - lastSendMs >= TELEMETRY_PERIOD_MS) {
    lastSendMs = millis();
    char buf[512];
    toJson(telemetry, buf, sizeof buf);
    events.send(buf, "t");
    Serial.println(buf);
  }
  delay(1);
}
