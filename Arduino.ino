#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include "DHT.h"

#define RX 2 // TX of ESP8266 is connected with Arduino pin 2
#define TX 3 // RX of ESP8266 is connected with Arduino pin 3
#define DHTPIN A0
#define METAL_DETECTOR_PIN 6 // Pin for metal detector
#define SWITCH_COOLING_PIN 7 // Pin for cooling switch
#define SWITCH_HEATING_PIN 8 // Pin for heating switch
#define PELTIER_HEATING_PIN 9 // Pin for Peltier module heating
#define PELTIER_COOLING_PIN 10 // Pin for Peltier module cooling
#define PULSE_SENSOR_PIN A1 // Pin for pulse sensor (analog input)
#define RGB_RED_PIN 11 // Pin for RGB LED red
#define RGB_BLUE_PIN 12 // Pin for RGB LED blue
#define RGB_GREEN_PIN 13 // Pin for RGB LED green
#define DHTTYPE DHT11

#define GPS_BAUD 9600 // Baud rate of GPS module
#define GPS_SERIAL Serial // Hardware serial port for GPS module

DHT dht(DHTPIN, DHTTYPE);
SoftwareSerial esp8266(RX, TX);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to 0x27 for a 16 chars and 2 line display
TinyGPSPlus gps;

String WIFI_SSID = "wifiname";      // WIFI NAME
String WIFI_PASS = "wifipassword";  // WIFI PASSWORD
String API_KEY = "W641VGEDU8MBKCGU"; // ThingSpeak API Key
String HOST = "api.thingspeak.com";
String PORT = "80";

unsigned long previousMillis = 0;
const long dataSendInterval = 30000; // Update ThingSpeak every 30 seconds

void setup() {
  Serial.begin(9600);
  esp8266.begin(115200);
  dht.begin();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  pinMode(METAL_DETECTOR_PIN, INPUT);
  pinMode(PELTIER_HEATING_PIN, OUTPUT);
  pinMode(PELTIER_COOLING_PIN, OUTPUT);
  pinMode(SWITCH_HEATING_PIN, INPUT_PULLUP);
  pinMode(SWITCH_COOLING_PIN, INPUT_PULLUP);
  pinMode(RGB_RED_PIN, OUTPUT);
  pinMode(RGB_BLUE_PIN, OUTPUT);
  pinMode(RGB_GREEN_PIN, OUTPUT);
  connectToWiFi();
  GPS_SERIAL.begin(GPS_BAUD); // Initialize hardware serial for GPS module
}

void loop() {
  // Code for reading data from GPS module using hardware serial
  float latitude;
  float longitude;

  readGPSData(latitude, longitude);
  
  unsigned long currentMillis = millis();
  int metalDetectorValue = digitalRead(METAL_DETECTOR_PIN);
  int pulseSensorValue = analogRead(PULSE_SENSOR_PIN);
  int peltierStatus = 0; // Initialize peltierStatus here

  // Read the state of the switches
  int heatingSwitchState = digitalRead(SWITCH_HEATING_PIN);
  int coolingSwitchState = digitalRead(SWITCH_COOLING_PIN);

  // Declare temperature and humidity variables
  float temperature;
  float humidity;

  // Update Peltier module and LED states based on switch states
  updatePeltierAndLEDState(heatingSwitchState, coolingSwitchState, peltierStatus);

  // Update ThingSpeak every dataSendInterval
  if (currentMillis - previousMillis >= dataSendInterval) {
    previousMillis = currentMillis;
    temperature = dht.readTemperature() * 15; // Read temperature from DHT11 sensor
    humidity = dht.readHumidity() * 2;       // Read humidity from DHT11 sensor

    // Display temperature and humidity
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print(" C");
    delay(2000); // Delay to allow reading

    lcd.setCursor(0, 1);
    lcd.print("Humid: ");
    lcd.print(humidity);
    lcd.print(" %");
    delay(2000); // Delay to allow reading

    // Display metal detection status and pulse sensor value
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Metal: ");
    lcd.print(metalDetectorValue == HIGH ? "Detd" : "Not Dtd");
    delay(2000); // Delay to allow reading

    lcd.setCursor(0, 1);
    lcd.print("Pulse: ");
    lcd.print(pulseSensorValue);
    delay(2000); // Delay to allow reading

    // Display GPS data
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Lat: ");
    lcd.print(latitude, 6);
    lcd.setCursor(0, 1);
    lcd.print("Long: ");
    lcd.print(longitude, 6);
    delay(2000); // Delay to allow reading

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Sending Data...");

    sendDataToThingSpeak(temperature, humidity, metalDetectorValue, pulseSensorValue, peltierStatus, latitude, longitude);

    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print("Data Sent...");
    delay(2000);

    // // Output to serial monitor
    // Serial.print("Temperature: ");
    // Serial.print(temperature);
    // Serial.println(" C");
    // Serial.print("Humidity: ");
    // Serial.print(humidity);
    // Serial.println(" %");
    // Serial.print("Metal Detector: ");
    //     Serial.println(metalDetectorValue == HIGH ? "Detected" : "Not Detected");
    // Serial.print("Pulse Sensor Value: ");
    // Serial.println(pulseSensorValue);
    // Serial.print("Peltier Status: ");
    // if (peltierStatus == 1) {
    //   Serial.println("Cooling");
    // } else if (peltierStatus == 2) {
    //   Serial.println("Heating");
    // } else {
    //   Serial.println("Neutral");
    // }
    // Serial.print("Latitude: ");
    // Serial.println(latitude);
    // Serial.print("Longitude: ");
    // Serial.println(longitude);
  }
}

void readGPSData(float &latitude, float &longitude) {
  while (GPS_SERIAL.available() > 0) {
    if (gps.encode(GPS_SERIAL.read())) {
      // Process GPS data here
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        longitude = gps.location.lng();
      }
    }
  }
}

void connectToWiFi() {
  sendCommand("AT", 5, "OK");
  sendCommand("AT+CWMODE=1", 5, "OK");
  sendCommand("AT+CWJAP=\"" + WIFI_SSID + "\",\"" + WIFI_PASS + "\"", 20, "OK");
}

void sendDataToThingSpeak(float temperature, float humidity, int metalDetectorValue, int pulseSensorValue, int peltierStatus, float latitude, float longitude) {
  String getData = "GET /update?api_key=" + API_KEY + "&field1=" + String(temperature) + "&field2=" + String(humidity) + "&field3=" + String(metalDetectorValue) + "&field4=" + String(pulseSensorValue) + "&field5=" + String(peltierStatus) + "&field6=" + String(latitude, 6) + "&field7=" + String(longitude, 6);

  sendCommand("AT+CIPMUX=0", 5, "OK");
  sendCommand("AT+CIPSTART=\"TCP\",\"" + HOST + "\"," + PORT, 15, "OK");

  // Add some delay before sending the data
  delay(2000);

  // Calculate the length of data to be sent
  int dataLength = getData.length();
  sendCommand("AT+CIPSEND=" + String(dataLength + 2), 5, ">"); // Add 2 bytes for "\r\n"
  esp8266.println(getData);
  delay(2000); // Increase delay to ensure data is sent properly
  sendCommand("AT+CIPCLOSE", 5, "OK");
}

void sendCommand(String command, int maxTime, char readReplay[]) {
  boolean found = false;
  int countTimeCommand = 0;

  Serial.print("AT command: ");
  Serial.print(command);
  Serial.print(" ");

  while (countTimeCommand < (maxTime * 1)) {
    esp8266.println(command);
    //delay(1000); // Add a delay here to give some time for the ESP8266 module to process the command
    if (esp8266.find(readReplay)) {
      found = true;
      break;
    }
    countTimeCommand++;
  }

  if (found) {
    Serial.println("OK");
  } else {
    Serial.println("Fail");
  }
}

void updatePeltierAndLEDState(int heatingSwitchState, int coolingSwitchState, int &peltierStatus) {
  if (heatingSwitchState == LOW && coolingSwitchState == HIGH) {
    digitalWrite(PELTIER_HEATING_PIN, LOW); // Turn on heating
    digitalWrite(PELTIER_COOLING_PIN, HIGH);
    digitalWrite(RGB_RED_PIN, LOW); // Set RGB LED to red
    digitalWrite(RGB_BLUE_PIN, HIGH); // Turn off blue LED
    digitalWrite(RGB_GREEN_PIN, HIGH); // Turn off green LED
    peltierStatus = 2; // Heating
  } else if (heatingSwitchState == HIGH && coolingSwitchState == LOW) {
    digitalWrite(PELTIER_HEATING_PIN, HIGH);
    digitalWrite(PELTIER_COOLING_PIN, LOW); // Turn on cooling
    digitalWrite(RGB_RED_PIN, HIGH); // Set RGB LED to blue
    digitalWrite(RGB_BLUE_PIN, LOW); // Turn on blue LED
    digitalWrite(RGB_GREEN_PIN, HIGH); // Turn off green LED
    peltierStatus = 1; // Cooling
  } else {
    digitalWrite(PELTIER_HEATING_PIN, HIGH);
    digitalWrite(PELTIER_COOLING_PIN, HIGH);
    digitalWrite(RGB_RED_PIN, HIGH); // Turn off red LED
    digitalWrite(RGB_BLUE_PIN, HIGH); // Turn off blue LED
    digitalWrite(RGB_GREEN_PIN, LOW); // Turn on green LED
    peltierStatus = 0; // Neutral
  }
}

