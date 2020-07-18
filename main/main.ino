#include <ESP8266WiFi.h>
#include <FastLED.h>

#define WIFI_SSID "f231fa"
#define WIFI_PASS "gain.037.barrier"
#define NUM_LEDS 300
#define LED_PIN 6

CRGB leds[NUM_LEDS];
int color = 0;
CRGB colors[3] = {CRGB::White, CRGB::Blue, CRGB::Green};
bool buttonIsPressed;

void setup() {
  //Enable flash to be used as a toggle button.
  pinMode(0, INPUT_PULLUP);
  buttonIsPressed = false;
  
  //Enable and turn off built in LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);
  Serial.println("Program Started");
  
  wifiConnect();

  ledSetup();
}

void loop() {
  // put your main code here, to run repeatedly:
  if(digitalRead(0) == 0 && !buttonIsPressed) {
    buttonPress();
  }
  if(buttonIsPressed && digitalRead(0) == 1) {
    buttonIsPressed = false;
  }
  delay(100);
}

void wifiConnect() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.println("Connecting...");
  }

  //Once connected
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Connected to Wifi!");
  Serial.println(WiFi.localIP());
}

void ledSetup() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(100);
  ledSetColor(color);
}

void ledSetColor(int color) {
  FastLED.showColor(colors[color]);
}

void buttonPress() {
  buttonIsPressed = true;
  Serial.println("Button Pressed!");
  color++;
  if(color == 3)
    color = 0;
  ledSetColor(color);
}
