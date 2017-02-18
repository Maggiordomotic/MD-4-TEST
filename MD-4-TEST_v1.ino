/*
 * Maggiordomotic (www.maggiordomotic.it)
 * 
 *          MD-4-TESTv1
 * 
 * This sketch is for ESP8266 chip
 * See our website for more information
 * 
 * modified 18 02 2017
 * by Maggiordomotic
 */

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/*
 *    Change the labels (between "" ) according to your settings :
 */
/**/  const char* esid =             "";    //SSID
/**/  const char* epass =            "";    //SSID's PWD
/**/  const char* mqttServerIP =     "";    //MQTT server IP
/**/
/**/  const char* mqttESPname =      "MDtest";              //ESP name
/**/  const char* mqttIN =           "/MD-4-TEST-IN";  
/**/  const char* mqttOUT =          "/MD-4-TEST-OUT";  
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/*
 * PIN mapping (change pin # according to your settings)
 * 
 * pin #4 e #5 can be used for i2c devices (default on wire.h)
 * pin #0 is huzzah hw switch & led used for reset (not change)
 * warning: pin #2 #15 #16 check ESP datasheet before use it !
*/
/**/  #define DHTPIN  14
/**/  #define pinDigInput 12
/**/  #define pinDigOutput 13
/**/  #define pinAnalogA0 A0
/**/  #define pinPushLedRed 0 //led red (all board) & internal switch (only huzzah)
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/*
 * Library
 */
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <MQTTClient.h>
#include <DHT.h>
#define DHTTYPE DHT22
WiFiClient net;
MQTTClient client;
DHT dht(DHTPIN, DHTTYPE, 22);
/*
 * Predefine for setup 
 */
void connect();
/*
 * Global variables
 */
float humidity, temp_f, temp_c;
unsigned long lastMillis = 0;
unsigned long previousMillis = 0;
const long interval = 2000;
int sensorValue = 0;
char receivei2c[10] = "";
/*
 * Serial print debug (remove "//" to enable debug)
 */
#define Sprint(a) // (Serial.print(a))
#define Sprintln(a) // (Serial.println(a))

/*
 * SETUP
 */

void setup() {

  Serial.begin(9600);
  Wire.begin();
  pinMode(pinDigOutput, OUTPUT);
  pinMode(pinDigInput, OUTPUT); 
  pinMode(pinPushLedRed, OUTPUT); 
  delay(10);
  Serial.print("Wifi begin...");
  WiFi.begin(esid, epass);
  Serial.println("done!");
  Serial.print("Client begin...");
  client.begin(mqttServerIP, net);
  Serial.println("done!");
  connect();
  
}//setup


void connect() {
  Serial.print("Connecting wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("done!");

  Serial.print("Connecting MQTT...");
  while (!client.connect(mqttESPname, "", "")) {
    Serial.print(".");
    }
  Serial.println("done!");
  
  Serial.print("Subscribe feed...");
  client.subscribe(mqttIN);
  Serial.println("done!");

}//connect

/*
 * VOID LOOP
 */
void loop() {
  client.loop();
  delay(10); // <- fixes some issues with WiFi stability

  if(!client.connected()) {
    connect();
  }

  // every 5 minutes
  if(millis() - lastMillis > 300000) {
    lastMillis = millis();
      /*
       * do something if needed
       */
  }

  int i=0;
  while (Wire.available()) { // slave may send less than requested
    //char c = Wire.read(); // receive a byte as character
    receivei2c[i] = Wire.read();
    i++;    
  }
  //answer from i2c devices must begin with "MD", delete 'if' to enable all msg rcv
  if((receivei2c[0] == 'M') && (receivei2c[1] == 'D')){
    Sprint("received i2c : ");
    Sprintln(receivei2c);
    String sendI2C = "I2C_";
    String str = receivei2c;
    Sprintln(str);
    sendI2C += str;
    client.publish(mqttOUT, sendI2C);
    // **!!WARNING for cmp check Wire.requestFrom value!!**
    // this is an example, change the code as your needed:
    if(strcmp(receivei2c, "MDtest01") == 0) Sprintln("test01 OK");
    else Sprintln("test01 NO");
    if(strcmp(receivei2c, "MDtest02") == 0) Sprintln("test02 OK");
    else Sprintln("test02 NO");
    receivei2c[0] = receivei2c[1] = ' ';
  }
}//loop

void gettemperature() {
  // Wait at least 2 seconds seconds between measurements.
  unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis >= interval) {
    // save the last time you read the sensor 
    previousMillis = currentMillis;   

    // Reading temperature for humidity takes about 250 milliseconds!
    // Sensor readings may also be up to 2 seconds 'old' (it's a very slow sensor)
    humidity = dht.readHumidity();          // Read humidity (percent)
    temp_f = dht.readTemperature(true);     // Read temperature as Fahrenheit
    // Check if any reads failed and exit early (to try again).
    if (isnan(humidity) || isnan(temp_f)) {
      Serial.println("Failed to read from DHT sensor!");
      return;
    }
  }
}//gettemperature


void messageReceived(String topic, String payload, char * bytes, unsigned int length) {

  Sprint("incoming: ");
  Sprint(topic);
  Sprint(" - ");
  Sprint(payload);
  Sprintln();

  if(topic == mqttIN){ 

    if(payload.substring(0,10) == "OnboardLed"){
          digitalWrite(pinPushLedRed, payload.substring(10, 11) == "1" ? HIGH : LOW);
    }

    if(payload.substring(0,6) == "DigOut"){
          digitalWrite(pinDigOutput, payload.substring(6, 7) == "1" ? HIGH : LOW);
    }

    if(payload == "DigIn"){
      String statoDigInput01 = "DigIn_" + String(digitalRead(pinDigInput));
      client.publish(mqttOUT, statoDigInput01);
      Sprintln(statoDigInput01);
    }
    
    if(payload == "AnIn"){
        int sensorValue = analogRead(pinAnalogA0);
        String trimmer_str = "AnIn_" + String(sensorValue);
        client.publish(mqttOUT, trimmer_str);
        Sprintln(trimmer_str);
    }
    
    if(payload == "ReadDHT"){
        gettemperature();
        String dhtData = "TempF_" + String(temp_f);
        client.publish(mqttOUT, dhtData);
        Sprintln(dhtData);
        dhtData = "Humid_" + String(humidity);
        client.publish(mqttOUT, dhtData);
        Sprintln(dhtData);
    }
    
    if(payload.substring(0,3) == "I2C"){
      String i2cmsg = (payload.substring(3, payload.length()));
      Wire.beginTransmission(8); 
      Wire.write(i2cmsg.c_str());
      Wire.endTransmission();    
      delay(50);
      Wire.requestFrom(8, 6);   // **!!WARNING "if cmp" answer must be 6 byte (or set different value)!!** 
      Sprintln(payload);
    }

    
  }//if(topic == mqttIN)
}//messageReceived


