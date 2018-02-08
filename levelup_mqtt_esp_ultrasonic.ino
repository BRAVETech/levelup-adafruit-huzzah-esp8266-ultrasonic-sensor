/***************************************************
  Based on the Adafruit MQTT Library ESP8266 Example written by Tony DiCola for Adafruit Industries
  and the Ultrasonic Sensor HC-SR04 and Arduino Tutorial written by Dejan Nedelkovski, www.HowToMechatronics.com
  
  Must use ESP8266 Arduino from:
  https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP Feather
  ----> https://www.adafruit.com/products/2821
  
  Adafruit invests time and resources providing their open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!
  
 Created by Christiaan Nel (guydelta) for BRAVETech, https://bigbrave.digital.
  
  MIT license, all text above must be included in any redistribution
  Author of this distribution: Christiaan Nel (guydelta)
****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
const float pi = 3.1415729;

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "... Your WIFI SSID here ..."
#define WLAN_PASS       "... Your WIFI Password here ..."

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  8883                   // use 8883 for SSL
#define AIO_USERNAME    "... Your Adafruit Username here ..."
#define AIO_KEY         "... Your Adafruit Access Key here ..."

/************************* Sensor Setup *********************************/
#define TRIG_PIN        13
#define ECHO_PIN        16

/************************* Tank Setup *********************************/
const int TANK_RADIUS = 38; //Radius in cm, max: 32767
const int TANK_HEIGHT = 185; //Full height in cm, max: 32767
const float CAL_FACTOR = 0.0344; //Calibration factor, this value is based on sound velocity

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
//WiFiClient client;
// or... use WiFiFlientSecure for SSL
WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'ultrasonic' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish ultrasonic = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/ultrasonic");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

void setup() {
  Serial.begin(115200);
  delay(10);

  Serial.println(F("Init Ultrasonic Sensor"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Set IO pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  float vol = sensor_ping();

  // Now we can publish stuff!
  Serial.print(F("\nSending ultrasonic val "));
  Serial.print(vol);
  Serial.print("...");
  if (! ultrasonic.publish(vol)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
  delay(5000);

  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
//   if(! mqtt.ping()) {
//     mqtt.disconnect();
//   }
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

// Function to ping the ultrasonic sensor and return volume
float sensor_ping() {
  float duration, distance;
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  duration = pulseIn(ECHO_PIN, HIGH);
  distance = (duration / 2) * CAL_FACTOR;
  Serial.print(distance);
  Serial.print("cm");
  if (distance <= 2) {
      distance = 0; //Too low to calculate, so assume no change
  } else if (distance >= 400 || distance >= TANK_HEIGHT) {
      distance = TANK_HEIGHT; //Assume that tank is empty if distance is out of range
  }
  return whatis_volume(distance);
}

// Function to calculate volume
float whatis_volume(float distance) {
    float h = TANK_HEIGHT - distance;
    if (h < 0) h = 0; //Something went wrong, we can't have a distance larger than tank height
    int r2 = TANK_RADIUS * TANK_RADIUS;
    float vol = pi * r2 * h;
    float liters = vol / 1000;
    return liters;
}
