#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include <WiFiClientSecureBearSSL.h>
#include <pt.h>

#define WIFI_SSID "f231fa"
#define WIFI_PASS "gain.037.barrier"
#define NUM_LEDS 300
#define BRIGHTNESS 100
#define LED_PIN 5

enum LED_MODE {
  OFF = -1,
  SOLID = 0,
  RAINBOW = 1,
  FLASH = 2
};

// LED Vars
int led_hues[NUM_LEDS];
CRGB leds[NUM_LEDS];
LED_MODE ledMode = OFF;
long timer;
bool isOn = false;
CRGB color;
int r, g, b;

long pressTime;
long deltaTimeButton = 0;

// Timer Vars
long getInitTime;
long deltaLEDTime;

pt ptRequest;

// HTTPS Vars
std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
HTTPClient http;
StaticJsonDocument<256> doc; //For Json Data Extraction

/**
 * Setup leds
 */
void initLeds() {
  FastLED.addLeds<WS2812B, LED_PIN>(leds, NUM_LEDS);
  FastLED.clear(true);
  FastLED.setBrightness(BRIGHTNESS);
}

/**
 * Flash leds for testing
 */
void ledTestCycle() {
  delay(500);
  CRGB colors[] = {CRGB::Red, CRGB::Green, CRGB::Blue};
  for(CRGB color : colors) {
    for(int i = 0; i < NUM_LEDS; i++) {
      leds[i] = color;
    }
    FastLED.show();
    delay(500);
  }
  FastLED.clear(true);
  delay(500);
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
  Serial.println("Connected!");

  //Once connected show LED
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("\nConnected to WiFi!");
  Serial.println(WiFi.localIP()); 
}

int httpRequestThread(struct pt* pt) {
  PT_BEGIN(pt);
  while(1) {
    PT_WAIT_UNTIL(pt, millis() - getInitTime > 7500);
    httpRequest("https://remote-leds.herokuapp.com/queue/dequeue");
  }
  

  PT_END(pt);
}

/**
 * Change the mode of the lds 
 */
void changeMode(int newMode) {
  if(newMode == 1 && ledMode == 1) return;

  ledMode = (LED_MODE)newMode;
  switch (ledMode) {
    case OFF:
      FastLED.clear(true);
      break;
    case SOLID:
      FastLED.setBrightness(BRIGHTNESS);
      extractColorFromJson();
      initLEDColor();
      break;
    case RAINBOW: 
      FastLED.setBrightness(BRIGHTNESS);
      initFade();
      break;
    case FLASH: 
      FastLED.setBrightness(BRIGHTNESS);
      extractColorFromJson();
      initFlash();
      break;
    default: break;
  }
}

/**
 * Turn off the LEDs (set all to black)
 */
void turnOff(int first, int last) {
  for (int i = first; i < last; i++) {
    leds[i].setHSV(255, 255, 255);
  }
  FastLED.show();
}

void setup() {
  // Init Serial
  Serial.begin(9600);
  while(!Serial) delay(250);
  Serial.println("");
  Serial.println("Program Started");
  Serial.flush();

  //Enable flash to be used as a toggle button.
  // pinMode(0, INPUT_PULLUP);
  // buttonIsPressed = false;

  // Init built-in led
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // Init leds
  initLeds();
  ledTestCycle();

  // Connect to Wifi and perform setup
  connectToWifi();
  client->setInsecure();

  // Initial http request
  httpRequest("https://remote-leds.herokuapp.com/queue/dequeue");               

  PT_INIT(&ptRequest);                                                                                                                                                                                                                                                                                           
}

/**
 * Handler for special LED logic every cycle
 */
void doLED() {
  switch (ledMode) {
    case RAINBOW: fade(); //If fade, move leds along. (maybe change this to a less rainbow-y and more gradual fade)
      break;
    case FLASH: flash();
    default: break; //Other settings do not require continuous updates
  }
}


void loop() {
  // GET latest led config every 10 seconds
  // Would be better to update this to a socket stream in the future
  PT_SCHEDULE(httpRequestThread(&ptRequest));
  doLED();
}

void extractColorFromJson() {
  const char* hex = doc["color"];
  char colorBuf[5];
  colorBuf[0] = '0';
  colorBuf[1] = 'x';

  colorBuf[2] = hex[1];
  colorBuf[3] = hex[2];
  color.r = strtol(colorBuf, NULL, 16); // Red

  colorBuf[2] = hex[3];
  colorBuf[3] = hex[4];
  color.g = strtol(colorBuf, NULL, 16); // Green

  colorBuf[2] = hex[5];
  colorBuf[3] = hex[6];
  color.b = strtol(colorBuf, NULL, 16); // Blue
}

void initLEDColor() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
  FastLED.show();
}

/**
 * Init ~1/3 of the color spectrum across all 300 leds
 */
void initFade() {
  // Serial.println("init fade");
  for(int hue = 0; hue < 100; hue++) {
    for(int led = 0; led < 3; led++) {
      leds[hue * 3 + led].setHue(hue);
      led_hues[hue * 3 + led] = hue;
    }
  }
  FastLED.show();
}

void fade() {
  for(int led = 0; led < NUM_LEDS; led++) {
    led_hues[led] = (led_hues[led] + 1) % 256;
    leds[led].setHue(led_hues[led]);
  }
  delay(30);
  FastLED.show();
}
void initFlash() {
  isOn = true;
  timer = millis();
  initLEDColor();
}

void moveLED() {
  CRGB temp = leds[299];
  for (int count = NUM_LEDS - 1; count > 0; count--) {
    leds[count] = leds[count - 1];
  }
  leds[0] = temp;
  delay(50); // Might need to process this using delta Time? Not sure if this would cause problems with reading from serial input
  FastLED.show();
}

void flash() {
  long currTime = millis();
  bool switched = false;
  if (currTime - timer > 1000) {
    isOn = !isOn;
    timer = currTime;
    switched = true;
  }
  if (switched) {
    if (isOn) {
      initLEDColor();
    } else {
      FastLED.clear(true);
    }
  }
}

void httpRequest(String url) { //Later change the GET request to occur within a while loop with http.connected to simulate a async function();
  // Serial.print("Requesting @ " + url + "...");
  http.begin(*client, url);
  int httpCode;
  httpCode = http.GET();
  // Serial.println(httpCode);

  if (httpCode == 200) {
    deserializeJson(doc, http.getString());
    const int modeNum = doc["mode"];
    changeMode(modeNum);
  }
  
  getInitTime = millis();
  //Dunno if i really need to support post request
  http.end();

}
