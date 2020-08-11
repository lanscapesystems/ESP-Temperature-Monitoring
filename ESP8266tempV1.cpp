#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

//edit lines 11-20 and 61-63. For IP addresses in lines 11-20 use .'s while for lines 61-63 use ,'s. Also edit lines 22-27 If not using a 10K NTC Thermistor.

const char* topic = "";   //topic to send temperature data to.
const char* ssid = "";    //WiFi SSID.
const char* password =  "";   //WiFi Password.
const char* mqttServer = "192.168.x.xxx";    //mosquitto server address
const int mqttPort = 1883;    //mosquitto server port. Mosquitto broker default is 1883.
const char* mqttUser = "";    //mosquitto user.
const char* mqttPassword = "";   //password for mosquitto user.
const char* mqtthostname = "";    //set mosquitto host name for the ESP. Must not be the same as any other clients.
const char* reconnectmsg = "";    //message to send to "reconnecttopic" upon mosquitto reconnection.
const char* reconnecttopic = "";    //topic to send "reconnectmsg" to.

#define THERMISTORNOMINAL 10000   //thermistor resistance.
#define TEMPERATURENOMINAL 25   //number that i dont remember what it is exactly.
#define BCOEFFICIENT 3950   //not sure about this one either.
#define SERIESRESISTOR 10000    //resistance of the resistor in series with the thermistor.
#define THERMISTORPIN A0    //analog pin the thermistor is connected to.
#define NUMSAMPLES 8    //number of samples to take.

int samples[NUMSAMPLES];
char* tempdat = "";
char reading[8];

WiFiClient espClient;
PubSubClient client(espClient);


void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  } 
}
 
long lastReconnectAttempt = 0;

boolean reconnect() {
  if (client.connect(mqtthostname)) {
    client.publish(reconnecttopic, reconnectmsg);   //send "reconnectmsg" to "reconnecttopic" upon mosquitto reconnect.
    //client.publish(reconnecttopic, WiFi.localIP);   //send the ESP's ip address to "reconnecttopic."
    client.subscribe(topic);
  }
  return client.connected();
}



void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  IPAddress ip(192,168,x,xxx);    //set the ESP's IP address. Make sure it is not conflicting with other devices.
  IPAddress gateway(192,168,x,x);   //set the gateway.
  IPAddress subnet(255,255,255,0);    //set the subnet mask. (255,255,255,0 for most home networks)
WiFi.config(ip, gateway, subnet);
  Serial.println(WiFi.localIP());
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected to the WiFi network");
 
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  delay(1500);
  lastReconnectAttempt = 0;
 
  while (!client.connected()) {
 
    if (client.connect(mqtthostname, mqttUser, mqttPassword )) {
 
      Serial.println("connected");  
 
    } else {
 
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
 
    }
  }
}



void loop(void) {
  if (!client.connected()) {
    long now = millis();
    if (now - lastReconnectAttempt > 5000) {
      lastReconnectAttempt = now;
      // Attempt to reconnect
      if (reconnect()) {
        lastReconnectAttempt = 0;
      }
    }
  } else {
    // Client connected

    client.loop();
  }
  uint8_t i;
  float average;
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(THERMISTORPIN);
   delay(10);
  }
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }

  average /= NUMSAMPLES;
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;
  
  float steinhart;
  steinhart = average / THERMISTORNOMINAL;
  steinhart = log(steinhart);
  steinhart /= BCOEFFICIENT;
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15);
  steinhart = 1.0 / steinhart;
  steinhart -= 273.15;
  steinhart = steinhart / 5;
  steinhart = steinhart * 9;
  steinhart = steinhart + 32;
  dtostrf(steinhart, 6, 0, tempdat);
  client.publish(topic, tempdat);     //publish the temperature in degrees Celcius to "topic"
  delay(5000);
  }
