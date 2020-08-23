#include <Arduino.h>
#include <ESPWiFiConfig.h>
#include <ArduinoJson.h>
#include <esp32cam.h>
#include <WiFi.h>
#include <MQTT.h>

#include "json_file.h"

DynamicJsonDocument config(5*1024);//5 KB

void camera_start(config){
  using namespace esp32cam;
  int width = config["camera"]["width"];
  int height = config["camera"]["height"];
  static auto resolution = esp32cam::Resolution::find(width, height);
  Config cfg;
  cfg.setPins(pins::AiThinker);
  cfg.setResolution(resolution);
  cfg.setBufferCount(config["camera"]["buffer_count"]);
  cfg.setJpeg(config["camera"]["jpeg_quality"]);

  bool ok = Camera.begin(cfg);
  Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");
}

void camera_publish(){
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    Serial.println("CAPTURE FAIL");
    return;
  }
  Serial.printf("CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(),
              static_cast<int>(frame->size()));
}

void setup() {
  Serial.begin(115200);

  load_config(config);

  camera_start(config);
  wifi_setup();

}

void loop() {
  camera_publish();
  delay(20000);
}

