#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

// WiFi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ThingSpeak
String apiKey = "AIzaSyA5lA6svMaLxZjF3oemNTUBjNlq4FsHwTo";
const char* server = "http://api.thingspeak.com/update";

// Pins
#define VOLTAGE_PIN 34
#define CURRENT_PIN 35
#define MACHINE_BTN 5
#define RESET_BTN 4
#define RED_PIN 25
#define BLUE_PIN 27

float voltage = 0;
float currentVal = 0;
float power = 0;
float energy = 0;

bool machineRunning = false;
bool lastMachineState = HIGH;
bool lastResetState = HIGH;

unsigned long previousMillis;
unsigned long lastUploadTime = 0;

void setup() {
  Serial.begin(115200);

  pinMode(MACHINE_BTN, INPUT_PULLUP);
  pinMode(RESET_BTN, INPUT_PULLUP);

  pinMode(RED_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi Connected!");

  previousMillis = millis();
}

void loop() {

  unsigned long currentMillis = millis();
  float timeDiff = (currentMillis - previousMillis) / 1000.0;
  previousMillis = currentMillis;

  int voltageRaw = analogRead(VOLTAGE_PIN);
  int currentRaw = analogRead(CURRENT_PIN);

  voltage = (voltageRaw / 4095.0) * 250.0;
  currentVal = (currentRaw / 4095.0) * 10.0;

  int machineState = digitalRead(MACHINE_BTN);
  int resetState = digitalRead(RESET_BTN);

  // Toggle machine
  if (machineState == LOW && lastMachineState == HIGH) {
    machineRunning = !machineRunning;
    delay(200);
  }
  lastMachineState = machineState;

  // Reset energy
  if (resetState == LOW && lastResetState == HIGH) {
    energy = 0;
    delay(200);
  }
  lastResetState = resetState;

  if(machineRunning) {
    power = voltage * currentVal;
    energy += power * timeDiff;

    digitalWrite(RED_PIN, LOW);
    digitalWrite(BLUE_PIN, HIGH);
  }
  else {
    power = 0;
    digitalWrite(RED_PIN, HIGH);
    digitalWrite(BLUE_PIN, LOW);
  }

  float energy_kWh = energy / 3600000.0;

  // LCD Display
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("P:");
  lcd.print(power,1);
  lcd.print("W");

  lcd.setCursor(0,1);
  lcd.print("E:");
  lcd.print(energy_kWh,4);
  lcd.print("kWh");

  // Upload every 15 seconds (ThingSpeak limit)
  if(millis() - lastUploadTime > 15000) {

    if(WiFi.status() == WL_CONNECTED) {
      HTTPClient http;

      String url = server;
      url += "?api_key=" + apiKey;
      url += "&field1=" + String(voltage);
      url += "&field2=" + String(currentVal);
      url += "&field3=" + String(power);
      url += "&field4=" + String(energy_kWh);

      http.begin(url);
      http.GET();
      http.end();

      Serial.println("Data Sent to ThingSpeak");
    }

    lastUploadTime = millis();
  }

  delay(500);
}