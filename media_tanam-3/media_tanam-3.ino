#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
 #include <SwitchManager.h>

const char* mqtt_server = "broker.hivemq.com";

WiFiManager wm;
SwitchManager myIncSwitch;
WiFiClient espClient;

PubSubClient client(espClient);

#define ONE_WIRE_BUS 2
const int SensorPin = A0;
const byte incSwitch = 3;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensorSuhu(&oneWire);
float suhuSekarang = 0;

unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
int value = 0;
int soilmoisturepercent=0;
int soilMoistureValue = 0;

const int AirValue = 790;   //you need to replace this value with Value_1
const int WaterValue = 390;  //you need to replace this value with Value_2
String msg;
unsigned long incShortPress   = 500UL; // 1/2 second
unsigned long incLongPress    = 2000UL;// 2 seconds 
const char* willTopic = "agriciatech/sensor/monitoring_tanam/1";

void setup_wifi() {
  delay(10);
  WiFi.mode(WIFI_STA);
 
  bool res;
  res = wm.autoConnect("Media_Tanam_1","password"); // password protected ap

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
  const char* willMsg = "[{\"value\": 0,\"id_sensor\":9,\"status\":\"offline\"},{\"value\": 0,\"id_sensor\":10,\"status\":\"offline\"}]";
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
    client.publish(willTopic, "[{\"value\": 0,\"id_sensor\":9,\"status\":\"online\"},{\"value\": 0,\"id_sensor\":10,\"status\":\"online\"}]");
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
  pinMode(BUILTIN_LED, OUTPUT);   // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  sensorSuhu.begin();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  myIncSwitch.begin (incSwitch, handleSwitchPresses); 
}

void sendDataSensor(void)
{
  soilMoistureValue = analogRead(SensorPin);  //put Sensor insert into soil
  soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
  if(soilmoisturepercent > 100){
    soilmoisturepercent = 100;
  }
  else if(soilmoisturepercent <0){
    soilmoisturepercent = 0;
  }

  sensorSuhu.requestTemperatures();

  suhuSekarang = sensorSuhu.getTempCByIndex(0);
  msg = "[{\"value\": "+ String(soilmoisturepercent) +",\"id_sensor\":9,\"status\":\"online\"},{\"value\":"+ String(suhuSekarang) + ",\"id_sensor\":10,\"status\":\"online\"}]";
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