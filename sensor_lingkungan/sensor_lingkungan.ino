#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Sodaq_SHT2x.h>
#include <SwitchManager.h>

const char* mqtt_server = "broker.hivemq.com";

WiFiManager wm;
SwitchManager myIncSwitch;
WiFiClient espClient;

PubSubClient client(espClient);

const byte incSwitch = 3;

float suhuSekarang = 0.0;
float kelembabanSekarang = 0.0;
float lux = 0.0;

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
int value = 0;

String msg;
unsigned long incShortPress   = 500UL; // 1/2 second
unsigned long incLongPress    = 2000UL;// 2 seconds 
const char* willTopic = "agriciatech/sensor/monitoring_tanaman/1";

void setup_wifi() {
  delay(10);
  WiFi.mode(WIFI_STA);
 
  bool res;
  res = wm.autoConnect("monitoring_lingkungan_1","password"); // password protected ap

}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
  Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected

  byte willQoS = 1;
  const char* willMsg = "[{\"value\": 0,\"id_sensor\":1,\"status\":\"offline\"},{\"value\": 0,\"id_sensor\":2,\"status\":\"offline\"},{\"value\": 0,\"id_sensor\":3,\"status\":\"offline\"}]";
  boolean willRetain = true;
 
  while (!client.connected()) {
  Serial.print("Attempting MQTT connection...");
  // Create a random client ID
  String clientId = "monitoring1";
  clientId += String(random(0xffff), HEX);
  // Attempt to connect
    
  if (client.connect(clientId.c_str(), willTopic, willQoS, willRetain, willMsg)) {
    Serial.println("connected");
    // Once connected, publish an announcement...
    client.publish(willTopic, "[{\"value\": 0,\"id_sensor\":1,\"status\":\"online\"},{\"value\": 0,\"id_sensor\":2,\"status\":\"online\"},{\"value\": 0,\"id_sensor\":3,\"status\":\"online\"}]");
    // ... and resubscribe
    //    client.subscribe("inTopic");
  } else {
    Serial.print("failed, rc=");
    Serial.print(client.state());
    Serial.println(" try again in 5 seconds");
    // Wait 5 seconds before retrying
    delay(5000);
  }
  }
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  Wire.begin();
  lightMeter.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  myIncSwitch.begin (incSwitch, handleSwitchPresses); 
}

void sendDataSensor(void)
{
  suhuSekarang = SHT2x.GetTemperature();
  kelembabanSekarang = SHT2x.GetHumidity();
  lux = lightMeter.readLightLevel();
  msg = "[{\"value\": "+ String(suhuSekarang) +",\"id_sensor\":1,\"status\":\"online\"},{\"value\":"+ String(kelembabanSekarang) + ",\"id_sensor\":2,\"status\":\"online\"},{\"value\":"+ String(lux) + ",\"id_sensor\":3,\"status\":\"online\"}]";
  //  timeClient.update();
  //  String created_at = String(timeClient.getFullFormattedTime());
  client.publish(willTopic,msg.c_str());
  //nilai = (char *) Serial.readString().c_str();
  //  nilai += '"' + created_at + '"' + "}";
  //Serial.println(nilai);
  //  if (!client.publish(MQTT_PUB_TOPIC, nilai.c_str(), false))
  //    pubSubErr(client.state());
  //
  //nilai = "";
}

void loop() {
  if (!client.connected()) {
  reconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 10000) {

  lastMsg = now;
  sendDataSensor();  
  }
}

void handleSwitchPresses(const byte newState, const unsigned long interval, const byte whichPin){
  switch (whichPin)
  {

  case incSwitch: 

    if (newState == HIGH)
    {
      if(interval <= incShortPress) 
      {
         ESP.restart();
      }
      else if(interval >= incLongPress) 
      {
       wm.resetSettings();
      }
    }

    break; 
  }
}