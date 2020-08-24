#include <Arduino.h>
#include <ESPWiFiConfig.h>
#include <ArduinoJson.h>
#include <esp32cam.h>
#include <WiFi.h>
#include <MQTT.h>

#include "json_file.h"

DynamicJsonDocument config(5*1024);//5 KB
MQTTClient mqtt(30*1024);// 30KB for jpg images
WiFiClient wifi;//needed to stay on global scope

bool low_power_mode = true;
RTC_DATA_ATTR uint32_t cycle_count = 0;

void timelog(String Text){
  Serial.println(String(millis())+" : "+Text);//micros()
}

void mqtt_start(DynamicJsonDocument &config){
  mqtt.begin(config["mqtt"]["host"],config["mqtt"]["port"], wifi);
  if(mqtt.connect(config["mqtt"]["client_id"])){
    Serial.println("mqtt>connected");
  }
}

void mqtt_publish_config(){
  String str_config;
  serializeJson(config,str_config);
  String str_topic = config["camera"]["base_topic"];
  mqtt.publish(str_topic+"/config",str_config);
  mqtt.loop();
}

void mqtt_loop(){
  mqtt.loop();
  if (!mqtt.connected()) {
    if(mqtt.connect(config["mqtt"]["client_id"])){
      Serial.println("mqtt>re-connected");
    }
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
  mqtt.publish(str_topic+"/count",String(cycle_count));
  str_topic += "/jpg";//cannot be added in the function call as String overload is missing
  mqtt.publish( str_topic.c_str(),
                reinterpret_cast<const char *>(frame->data()),
                frame->size());
}

void setup() {
  
  Serial.begin(115200);
  timelog("Boot ready @ cycle "+String(cycle_count));
  

  //TODO low_power_mode from pio switch

  load_config(config,!low_power_mode);
  timelog("config loaded");

  camera_start(config);
  timelog("camera started");
  wifi_setup();//no config wifi to protect writing credentials in dev files
  timelog("wifi setup");
  mqtt_start(config);
  if(cycle_count == 0){
    mqtt_publish_config();
  }
  timelog("setup() done");

}

void loop() {
  cycle_count++;
  timelog("loop start cycle "+String(cycle_count));
  camera_publish();
  timelog("camera publish done");
  mqtt_loop();
  timelog("mqtt loop done");
  if(low_power_mode){
    timelog("entering deep sleep");
    Serial.flush();
    delay(500);//finish sending the message TODO check transmission completion
    esp_deep_sleep(10*1000*1000);
  }else{
    delay(10000);
  }
}
