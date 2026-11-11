
#define BLYNK_TEMPLATE_ID "TMPL3jQShAtSn"
#define BLYNK_TEMPLATE_NAME "Smart Agriculture"
#define BLYNK_AUTH_TOKEN "KXRfvMKJeQRT_23EC1LKDI3bFzzlTn37"

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Wi-Fi details
char ssid[] = "Airtel_Jaya";
char pass[] = "Airtel@123";

// Sensor pins
#define SOIL_PIN A0
#define DHT_PIN D4
#define DHT_TYPE DHT11

// Relay pins
#define PUMP_RELAY D5
#define FAN_RELAY  D6
#define LIGHT_RELAY D7

// Relay module assumed ACTIVE LOW
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// OLED settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDRESS 0x3C

DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 display(
  SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1
);

BlynkTimer timer;

// Soil calibration values: adjust after testing your sensor
int dryValue = 850;
int wetValue = 350;

// Automatic control settings
bool pumpState = false;
bool fanState = false;
bool lightState = false;

float temperature = 0;
float humidity = 0;
int soilMoisture = 0;

void setRelay(int pin, bool state) {
  digitalWrite(pin, state ? RELAY_ON : RELAY_OFF);
}

// Blynk V3 button controls grow light
BLYNK_WRITE(V3) {
  lightState = param.asInt();
  setRelay(LIGHT_RELAY, lightState);
}

BLYNK_CONNECTED() {
  Blynk.syncVirtual(V3);
}

void readSensors() {

  // Read soil moisture
  int soilRaw = analogRead(SOIL_PIN);

  soilMoisture = map(
    soilRaw, dryValue, wetValue, 0, 100
  );

  soilMoisture = constrain(soilMoisture, 0, 100);

  // Read DHT11
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  if (!isnan(t) && !isnan(h)) {
    temperature = t;
    humidity = h;
  }

  // Automatic water pump control
  if (soilMoisture < 50 && !pumpState) {
    pumpState = true;
    setRelay(PUMP_RELAY, true);
  }

  if (soilMoisture >= 70 && pumpState) {
    pumpState = false;
    setRelay(PUMP_RELAY, false);
  }

  // Automatic fan control with hysteresis
  if (temperature > 35 && !fanState) {
    fanState = true;
    setRelay(FAN_RELAY, true);
  }

  if (temperature <= 33 && fanState) {
    fanState = false;
    setRelay(FAN_RELAY, false);
  }

  // Send data to Blynk
  if (Blynk.connected()) {
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, humidity);
    Blynk.virtualWrite(V2, soilMoisture);
  }

  // OLED display
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SMART AGRICULTURE");

  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);

  display.setCursor(0, 19);
  display.print("Temp: ");
  display.print(temperature);
  display.println(" C");

  display.setCursor(0, 31);
  display.print("Humidity: ");
  display.print(humidity);
  display.println(" %");

  display.setCursor(0, 43);
  display.print("Soil: ");
  display.print(soilMoisture);
  display.println(" %");

  display.setCursor(0, 55);
  display.print("Pump:");
  display.print(pumpState ? "ON " : "OFF");
  display.print(" Fan:");
  display.print(fanState ? "ON" : "OFF");

  display.display();

  Serial.print("Soil raw: ");
  Serial.print(soilRaw);
  Serial.print(" | Soil: ");
  Serial.print(soilMoisture);
  Serial.print("% | Temp: ");
  Serial.println(temperature);
}

void setup() {

  Serial.begin(9600);

  // Set relay outputs OFF immediately
  pinMode(PUMP_RELAY, OUTPUT);
  pinMode(FAN_RELAY, OUTPUT);
  pinMode(LIGHT_RELAY, OUTPUT);

  setRelay(PUMP_RELAY, false);
  setRelay(FAN_RELAY, false);
  setRelay(LIGHT_RELAY, false);

  // Sensors
  dht.begin();

  // OLED
  Wire.begin(D2, D1); // SDA, SCL

  if (!display.begin(
    SSD1306_SWITCHCAPVCC, OLED_ADDRESS
  )) {
    Serial.println("OLED not detected!");
  } else {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 20);
    display.println("SMART AGRICULTURE");
    display.println("Starting...");
    display.display();
  }

  // Blynk connection
  Blynk.begin(
    BLYNK_AUTH_TOKEN,
    ssid,
    pass
  );

  // Read sensors every 2 seconds
  timer.setInterval(2000L, readSensors);
}

void loop() {
  Blynk.run();
  timer.run();
}
