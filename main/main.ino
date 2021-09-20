#include <ArduinoJson.h>
#include "enums.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <NeoPixelBus.h>
#include <pt.h>
#include <WiFiClientSecureBearSSL.h>

#define WIFI_SSID "WolfieNet-IoT"
#define WIFI_PASS NULL
#define REQUEST_INTERVAL 5000
#define MINS_TO_SLEEP 5 

#define BRIGHTNESS 100
#define LIGHTNESS .34f
#define NUM_LEDS 300
#define LED_PIN 4
#define DEFAULT_LED_MODE FADE

// LED Vars
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(NUM_LEDS);
LED_MODE ledMode = DEFAULT_LED_MODE;

const String API = "https://remote-leds.herokuapp.com/queue";

// Timer Vars
long lastRequestTime;

pt ptRequest;

// HTTPS Vars
std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
HTTPClient http;
StaticJsonDocument<256> doc; //For Json Data Extraction

void httpRequest(String url) {
  Serial.print("Requesting @ " + url + "...");
  http.begin(*client, url);
  int httpCode;
  httpCode = http.GET();
  Serial.println(httpCode);

  if (httpCode == 200) {
    deserializeJson(doc, http.getString());
    ledMode = (LED_MODE) doc["mode"];
    Serial.println(ledMode);
    initLed();
  }else if(httpCode == 400) {
    Serial.println("No effect queued");
  }
  
  lastRequestTime = millis();
  http.end();
}

int httpRequestThread(struct pt* pt) {
  PT_BEGIN(pt);
  while(1) {
    PT_WAIT_UNTIL(pt, millis() - lastRequestTime > REQUEST_INTERVAL);
    httpRequest(API + "/dequeue");
  }
  PT_END(pt);
}

/**
 * Flash through red green and blue
 */
void ledTestCycle() {
  RgbColor colors[] = {RgbColor(BRIGHTNESS,0,0), RgbColor(0,BRIGHTNESS,0), RgbColor(0,0,BRIGHTNESS), RgbColor(0,0,0)};
  for(RgbColor color : colors) {
    for(int i = 0; i < NUM_LEDS; i++) {
      strip.SetPixelColor(i, color);
    }
    strip.Show();
    delay(1000);
  }
}

/**
 * Connect to the wifi and flash the led while connecting
 */
void connectToWifi() {
  //Attempt to connect
  bool ledIsOn = true;
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    int ledVal = ledIsOn ? HIGH : LOW;
    ledIsOn = !ledIsOn;
    digitalWrite(LED_BUILTIN, ledVal);
    delay(1000);
  }
  Serial.println("\nConnected to WiFi!");
  Serial.println(WiFi.localIP()); 
}

void turnOff() {
  for(int i = 0; i < NUM_LEDS; i++) {
    RgbColor black(0, 0, 0);
    strip.SetPixelColor(i, black);
  }
  strip.Show();
}

void setColor(RgbColor color) {
  color.Darken(50);
  for(int i = 0; i < NUM_LEDS; i++) {
    strip.SetPixelColor(i, color);
  }
  strip.Show();
}

void initSmoothFadeBands(int startHue, int finalHue, int repeats) {
  if(repeats > 10 || repeats <= 0 || NUM_LEDS % repeats != 0) repeats = 1;

  int bandLength = NUM_LEDS / repeats;
  for(int cycle = 0; cycle < repeats; cycle++) {
    for(int led = 0; led < bandLength / 2; led++) {
      int hue; 
      if(startHue <= finalHue) {
        hue = (float) led / (bandLength / 2) * (finalHue - startHue) + startHue;
      }else {
        hue = (float) led / (bandLength / 2) * (finalHue + 360 - startHue) + startHue;
      }
      hue = hue % 360;
      strip.SetPixelColor(cycle * bandLength + led, HsbColor((float) hue / 360, 1, LIGHTNESS));
      strip.SetPixelColor((cycle + 1) * bandLength - led - 1, HsbColor((float) hue / 360, 1, LIGHTNESS));
    }
  }
  strip.Show();
}

void initFullFade() {
  initSmoothFadeBands(0, 359, 1);
}

void initCoolFade() {
  initSmoothFadeBands(170, 290, 2);
}

void initWarmFade() {
  initSmoothFadeBands(340, 45, 2);
}

void initLed() {
  switch(ledMode) {
    case OFF: {
      turnOff();
    }
      break;
    case SOLID: {
      // Extract color from JSON doc
      HtmlColor color;
      const char* colorString = doc["color"];
      color.Parse<HtmlColorNames>(colorString);
      setColor(RgbColor(color));
    }
      break;
    case FADE: {
      FADE_TYPE fadeType = (FADE_TYPE) doc["type"];
      if(fadeType == NULL) fadeType = FULL;
      switch(fadeType) {
        case FULL: 
          initFullFade();
          break;
        case COOL:
          initCoolFade();
          break;
        case WARM:
          initWarmFade();
          break;
        case CUSTOM:
        default:
          break;
      }
    }
    default: 
      break;
  }
}

void doLed() {
  static long flashTime = 0;
  switch (ledMode) {
    case FADE: {
      strip.RotateLeft(1);
      strip.Show();
      delay(35);
    }
      break;
    case FLASH: {
      if(millis() - flashTime > 2000) {

      }
    }
    default: break; //Other settings do not require continuous updates
  }
}

void setup() {
  // Init Serial
  Serial.begin(9600);
  while(!Serial)
  Serial.println("Program Started");
  Serial.flush();
  
  // Init built-in led
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  // Set up wifi
  // connectToWifi();
  bool ledIsOn = true;
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, ledIsOn);
    ledIsOn = !ledIsOn;
    delay(500);
  }
  digitalWrite(LED_BUILTIN, HIGH);
  client->setInsecure();

  // Init leds
  strip.Begin();
  strip.Show();
  ledTestCycle();

  httpRequest(API + "/current");           
  PT_INIT(&ptRequest);                                                                                                                                                                                                                                                                                           
}

void loop() {
  PT_SCHEDULE(httpRequestThread(&ptRequest));
  doLed();
}
