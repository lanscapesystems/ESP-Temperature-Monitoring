#include <Arduino.h>
#if defined(ESP32)
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#define DEVICE "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#define DEVICE "ESP8266"
#endif

#include <InfluxDbClient.h>

#define THERMISTORNOMINAL 10000   //thermistor resistance.
#define TEMPERATURENOMINAL 25   //number that i dont remember what it is exactly.
#define BCOEFFICIENT 3950   //not sure about this one either.
#define SERIESRESISTOR 10000    //resistance of the resistor in series with the thermistor.
#define NUMSAMPLES 8    //number of samples to take.
#define THERMISTORPIN A0

int samples[NUMSAMPLES];
char* tempdat = "";
char reading[8];


#define WIFI_SSID ""     //WiFi SSID
#define WIFI_PASSWORD ""      //WiFi Password

#define INFLUXDB_URL ""      // InfluxDB server address, e.g. http://192.168.0.156:8086
#define INFLUXDB_DB_NAME ""     // InfluxDB database name 

#define INFLUXDB_USER ""      //InfluxDB user
#define INFLUXDB_PASSWORD ""     //InfluxDB password


InfluxDBClient client(INFLUXDB_URL, INFLUXDB_DB_NAME);

Point sensor("temperature");      // Data point

void setup() {
  Serial.begin(115200);

  // Connect WiFi
  Serial.println("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  while (wifiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();

  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PASSWORD);

  // Add constant tags - only once (optional)
  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void loop() {
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
  steinhart = steinhart * 9 / 5 + 32;     //convert to Farenheit. Remove this line to write temperature in degrees Celcius.
  
  sensor.clearFields();
  sensor.addField("temp", steinhart);
  Serial.println(sensor.toLineProtocol());
  // If no Wifi signal, try to reconnect it
  if ((WiFi.RSSI() == 0) && (wifiMulti.run() != WL_CONNECTED))
    Serial.println("Wifi connection lost");
  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
  }
  delay(1000);
