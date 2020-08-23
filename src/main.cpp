#include <Arduino.h>
#include <ESPWiFiConfig.h>
#include <ArduinoJson.h>
#include <esp32cam.h>
#include <WiFi.h>
#include <MQTT.h>

#include "json_file.h"

DynamicJsonDocument config(5*1024);//5 KB
MQTTClient mqtt(20*1024);// 20KB for jpg images
WiFiClient wifi;//needed to stay on global scope

bool low_power_mode = true;

void mqtt_try_connect(){
  mqtt.connect(config["mqtt"]["client_id"]);
  if(mqtt.connected()){
    Serial.println("mqtt>connected");
    if(!low_power_mode){
      String str_config;
      serializeJson(config,str_config);
      String str_topic = config["camera"]["base_topic"];
      bool res = mqtt.publish(str_topic+"/config",str_config);
      Serial.printf("publish result = %d\n",res);
      mqtt.loop();
    }
  }
}

void mqtt_start(DynamicJsonDocument &config){
  mqtt.begin(config["mqtt"]["host"],config["mqtt"]["port"], wifi);
  mqtt_try_connect();
}

void mqtt_loop(){
  mqtt.loop();
  if (!mqtt.connected()) {
    mqtt_try_connect();
  }
}

void camera_start(DynamicJsonDocument &config){
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
    Serial.println("camera> CAPTURE FAIL");
    return;
  }
  Serial.printf("camera> CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(),
              static_cast<int>(frame->size()));
  String str_topic = config["camera"]["base_topic"];
  Serial.printf("mqtt> publishing on %s\n",str_topic.c_str());
  mqtt.publish(str_topic+"/jpg","main image jpg content will go here");
}

void setup() {
  Serial.begin(115200);

  //TODO low_power_mode from pio switch

  load_config(config,!low_power_mode);

  camera_start(config);
  wifi_setup();//no config wifi to protect writing credentials in dev files
  mqtt_start(config);

}

void loop() {
  camera_publish();
  mqtt_loop();
  delay(500);//finish sending the message TODO check transmission completion
  esp_sleep_enable_timer_wakeup(10*1000*1000);
  esp_deep_sleep_start();
  //delay(10000);

}
