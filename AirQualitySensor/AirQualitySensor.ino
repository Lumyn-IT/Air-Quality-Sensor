#include <SoftwareSerial.h>  //SoftwareSerial Library
SoftwareSerial pmsSerial(D2, D3);

#include "DHT.h" //DHT Library
#define pinDHT D5 // Sensor Pin
DHT dht(pinDHT, DHT22); //Create a sensor from the library, setting the type DHT22, which uses the same communication type as the AM2120

//OTA libraries
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//MQTT libraries
#include <PubSubClient.h>

//OTA variable declaration
#ifndef STASSID
#define STASSID "YOUR_WIFI_SSID_NETWORK"
#define STAPSK "YOUR_WIFI_PASSOWRD"
#endif
String newHostname = "YOUR_DESIDERED_HOSTNAME";

//Wifi variables configuration
const char* ssid = STASSID;
const char* password = STAPSK;

int IsConnected = 0;

//const int WifiLed = D2;    //Builtin LED 

//MQTT variable declaration
// Set MQTT server configuration
char mqttServer[]  = "YOUR_MQTT_BROKER_IP_ADDRESS"; //Example: "192.168.1.10" Change it for your MQTT server IP or network name
int mqttPort  = 1883; //1883 is the default port for MQTT. Change if necessary
char deviceId[]  = "YOUR_DEVICE_NAME"; //every device should have a different name
char topic[]  = "YOUR_TOPIC_NAME"; //the topic should be different for each device as well
char mqttUser[]  = "YOUR_MQTT_BROKER_USERNAME";  //Example: "meetoo"
char mqttPassword[]  = "YOUR_MQTT_BROKER_PASSWORD";  //Example: "mypassword"

unsigned long lastOperation;
const long MIN_OPERATION_INTERVAL = 1000L; // Define time between relay commands
int status = WL_IDLE_STATUS;
signed long now; // ***
String message; // ***

//Start services
WiFiClient wifiClient;
PubSubClient client(wifiClient);

//-------------
//#define DEBUG //Uncomment to enable debugging
//-------------

void connect(); 
void blinkled();
void serialDebug();

void setup() {
  
#ifdef DEBUG
  Serial.begin(115200);
#endif

//Pinmode declaration
//pinMode(WifiLed, OUTPUT);
pinMode(LED_BUILTIN, OUTPUT);  //Comment this line if you want permanently turn off blue led ESP8266 onboard.

//OTA Start services
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

//ArduinoOTA.setPort(8266);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  // sensor baud rate is 9600
  pmsSerial.begin(9600); //Start of communication with the PMS5003 sensor

  dht.begin(); //Start of communication with the DHT sensor

}

//PMS5003 variable declaration
struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};
 
struct pms5003data data;

//DHT variables declaration    
  float temperature = 0; //Temperature variable
  float humidity = 0; //Humidity variable

void loop() {
//Debug control
#ifdef DEBUG
  serialDebug();
#endif

  //Connection control
if ( !client.connected() ) {
    connect();
  }
  client.loop();

//Wifi check
if (status = WL_CONNECTED){
  IsConnected = 1;
  digitalWrite(LED_BUILTIN, LOW);
  client.publish("AirQualitySensor/WifiConnected", String(1).c_str(), true);
} else {
  IsConnected = 0;
  digitalWrite(LED_BUILTIN, HIGH);
  client.publish("AirQualitySensor/WifiConnected", String(0).c_str(), true);
}
/*
//Wifi led control
if (IsConnected = 1){
  digitalWrite(WifiLed, LOW);
  client.publish("AirQualitySensor/WifiConnected", String(1).c_str(), true);
}
else {
  digitalWrite(WifiLed, HIGH);
  client.publish("AirQualitySensor/WifiConnected", String(0).c_str(), true);
}
*/
//Voids
  ArduinoOTA.handle();

  if (readPMSdata(&pmsSerial)) {
    // reading data was successful!

  //Concentration Units (standard)
  client.publish("AirQualitySensor/PMS5003/ConcentrationUnitsStandard/PM1.0", String(data.pm10_standard).c_str(), true);
  client.publish("AirQualitySensor/PMS5003/ConcentrationUnitsStandard/PM2.5", String(data.pm25_standard).c_str(), true);
  client.publish("AirQualitySensor/PMS5003/ConcentrationUnitsStandard/PM10", String(data.pm100_standard).c_str(), true);


  //Concentration Units (environmental)
  client.publish("AirQualitySensor/PMS5003/ConcentrationUnitsEnvironmental/PM1.0", String(data.pm10_env).c_str(), true);
  client.publish("AirQualitySensor/PMS5003/ConcentrationUnitsEnvironmental/PM2.5", String(data.pm25_env).c_str(), true);
  client.publish("AirQualitySensor/PMS5003/ConcentrationUnitsEnvironmental/PM10", String(data.pm100_env).c_str(), true);

  //Particles > X.Xum / 0.1L air
  client.publish("AirQualitySensor/PMS5003/Particles/P0.3", String(data.particles_03um).c_str(), true);
  client.publish("AirQualitySensor/PMS5003/Particles/P0.5", String(data.particles_05um).c_str(), true);
  client.publish("AirQualitySensor/PMS5003/Particles/P1.0", String(data.particles_10um).c_str(), true);
  client.publish("AirQualitySensor/PMS5003/Particles/P2.5", String(data.particles_25um).c_str(), true);
  client.publish("AirQualitySensor/PMS5003/Particles/P5.0", String(data.particles_50um).c_str(), true);
  client.publish("AirQualitySensor/PMS5003/Particles/P10.0", String(data.particles_100um).c_str(), true);
 

  }

  AM2120(); //Start AM2120 void

    //Check whether the read values ​​are valid using the isnan function
  if (isnan(temperature) || isnan(humidity)) { 

    //Serial.println("Fehler beim Lesen des Sensors");
    //delay(800);
    return;
  
  } 
  
  else {

  client.publish("AirQualitySensor/AM2120/Temperature", String(temperature).c_str(), true);
  client.publish("AirQualitySensor/AM2120/Humidity", String(humidity).c_str(), true);

  }


 blinkled();  //Comment this line if you don't want wifi led blink during transmission.
 delay(1000);

}


//PMS5003 void
boolean readPMSdata(Stream *s) {
  if (! s->available()) {
    return false;
  }
  
  // Read a byte at a time until we get to the special '0x42' start-byte
  if (s->peek() != 0x42) {
    s->read();
    return false;
  }
 
  // Now read all 32 bytes
  if (s->available() < 32) {
    return false;
  }
    
  uint8_t buffer[32];    
  uint16_t sum = 0;
  s->readBytes(buffer, 32);
 
  // get checksum ready
  for (uint8_t i=0; i<30; i++) {
    sum += buffer[i];
  }
 
  /* debugging
  for (uint8_t i=2; i<32; i++) {
    Serial.print("0x"); Serial.print(buffer[i], HEX); Serial.print(", ");
  }
  Serial.println();
  */
  
  // The data comes in endian'd, this solves it so it works on all platforms
  uint16_t buffer_u16[15];
  for (uint8_t i=0; i<15; i++) {
    buffer_u16[i] = buffer[2 + i*2 + 1];
    buffer_u16[i] += (buffer[2 + i*2] << 8);
  }
 
  // put it into a nice struct :)
  memcpy((void *)&data, (void *)buffer_u16, 30);
 
  if (sum != data.checksum) {
    Serial.println("Checksum failure");
    return false;
  }
  // success!
  return true;
}

//DHT void
void AM2120(){

  temperature = dht.readTemperature(); //Temperatur in eine Variable einlesen
  humidity = dht.readHumidity(); //Luftfeuchtigkeit in eine Variable einlesen

}

// MQTT commands management (the nodeMCU stays listening on topic)
void callback(char* topic, byte* payload, unsigned int length) {

    // Inhibits commands if time between commands is < MIN_OPERATION_INTERVAL
    signed long now = millis();
    long deltaTime = now - lastOperation;
    if (deltaTime > MIN_OPERATION_INTERVAL) {
      String message = ""; // *** 
      for (int i = 0; i < length; i++) {
        message = message + (char)payload[i];
      }

  // Managing incoming commands (topic: relay_command/.........) and status LED (topic: relay_status/.........)
      
      // --------- Relay 1 ---------------
    //  if(message == "Relay_ON") { 
    //    RelayCommand = 1;
    //   lastOperation = now;

    //  } else if(message == "Relay_OFF") {
    //    RelayCommand = 0;
    //    lastOperation = now;
    //  }

    //  } else {
  }
}
// ---------------------------------------
void connect() {
  
  while (!client.connected()) {
    
    status = WiFi.status();
    // Connection status blue LED activation
    //wifi_status_led_install(WIFI_LED_IO_NUM, WIFI_LED_IO_MUX, FUNC_GPIO0); 

    if ( status != WL_CONNECTED) {
      WiFi.hostname(newHostname.c_str()); // set hostname
      WiFi.begin(STASSID, STAPSK);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
      Serial.println();
      Serial.print("Connected to ");
      Serial.println(STASSID);
      Serial.print("IP address: "); 
      Serial.println(WiFi.localIP());
      Serial.print("Hostname: "); 
      Serial.println(WiFi.hostname());

      // Keep blue led active until connected
      digitalWrite(LED_BUILTIN, LOW); // physical Blue LED on the unit
      IsConnected = 1;
      client.publish("AirQualitySensor/WifConnected", String(1).c_str(), true); // Connected LED on the dashboard

      // print signal strenght
      Serial.println("------------");
      Serial.println("Signal strenght [dBm]");
      Serial.println(WiFi.RSSI());
      Serial.println("------------");

    }
    // Connection to MQTT server
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
    if (client.connect(deviceId, mqttUser, mqttPassword)) {
      client.subscribe(topic);
      Serial.println("Connected to MQTT Server");
    } else {
      Serial.print("[FAILED] [ rc = ");
      Serial.print(client.state() );
      Serial.println(" : retrying in 5 seconds]");
      delay(5000);
    }
  }
}
// ---------------------------------------


void blinkled(){
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
}


//Serial print void
void SerialDebug(){

  //Serial print PMS5003 value
  Serial.println();
  Serial.println("---------------------------------------");
  Serial.println("Concentration Units (standard)");
  Serial.print("PM 1.0: "); Serial.print(data.pm10_standard);
  Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_standard);
  Serial.print("\t\tPM 10: "); Serial.println(data.pm100_standard);
  Serial.println("---------------------------------------");
  Serial.println("Concentration Units (environmental)");
  Serial.print("PM 1.0: "); Serial.print(data.pm10_env);
  Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_env);
  Serial.print("\t\tPM 10: "); Serial.println(data.pm100_env);
  Serial.println("---------------------------------------");
  Serial.print("Particles > 0.3um / 0.1L air:"); Serial.println(data.particles_03um);
  Serial.print("Particles > 0.5um / 0.1L air:"); Serial.println(data.particles_05um);
  Serial.print("Particles > 1.0um / 0.1L air:"); Serial.println(data.particles_10um);
  Serial.print("Particles > 2.5um / 0.1L air:"); Serial.println(data.particles_25um);
  Serial.print("Particles > 5.0um / 0.1L air:"); Serial.println(data.particles_50um);
  Serial.print("Particles > 10.0 um / 0.1L air:"); Serial.println(data.particles_100um);
  Serial.println("---------------------------------------");

  //Serial print AM2120 value
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(" °C, ");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%.");
}
