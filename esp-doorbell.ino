#include <MyConfig.h> // credentials, servers, ports
#define MQTT_TOPIC "doorbell"
#define MQTT_TOPIC_SET "doorbell/set"
#include "wifi_ota_mqtt.h"
#include <ArduinoJson.h> // https://arduinojson.org

#define MIC_PIN    D5

// settings via MQTT_TOPIC_SET
unsigned int interval = 100; // check every interval ms and let hits fall off
unsigned int threshold = 2000; // publish mqtt msg once we reach this number of hits
unsigned int msg_debounce = 3000; // do not publish another msg for this time (ms)
unsigned int slope = 2; // divide by this factor each interval to drop hits back to 0

#define UPD(var)   if(doc[#var]) { var = doc[#var]; Serial.printf("%s = %u;\n", #var, var); }

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("MQTT message on topic %s with payload ", topic);
  for(int i = 0; i < length; i++){
    Serial.print((char) *(payload+i));
  }
  Serial.println();
  if (strcmp(topic, MQTT_TOPIC_SET) == 0) {
    StaticJsonDocument<120> doc; // size 102 determined with https://arduinojson.org/v6/assistant/
    deserializeJson(doc, payload);
    UPD(interval);
    UPD(threshold);
    UPD(msg_debounce);
    UPD(slope);
  }
}

void setup() {
  Serial.begin(38400);
  Serial.println("setup");
  setup_WiFi();
  // setup_OTA();
  mqtt.setCallback(mqtt_callback);
  setup_MQTT();

  pinMode(MIC_PIN, INPUT);
}


unsigned int a; // accumulated microphone threshold hits
unsigned long lastMillis;
unsigned long lastMsg;

void loop() {
  ArduinoOTA.handle();
  mqtt.loop();
  // if (!mqtt.connected()) setup_MQTT();

  // Serial.println(digitalRead(MIC_PIN));
  byte b = digitalRead(MIC_PIN);
  if (!b) a++;
  if(millis() - lastMillis > interval) {
      lastMillis = millis();
      Serial.println(a);
      if (a >= threshold && millis() - lastMsg > msg_debounce) {
        mqtt.publish(MQTT_TOPIC, json("\"hits\": %u, \"millis\": %lu", a, millis()));
        Serial.printf("publish %s", buf);
        lastMsg = millis();
      }
      a /= slope;
  }
}
