#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <FastLED.h>
#include <ArduinoJson.h>

#define WIFI_SSID "f231fa"
#define WIFI_PASS "gain.037.barrier"
#define NUM_LEDS 300
#define LED_PIN 6

CRGB leds[NUM_LEDS];
int ledMode = -1;
long timer;
bool isOn = false;
int r, g, b;

bool buttonIsPressed;
long pressTime;
long deltaTimeButton = 0;

HTTPClient http;
StaticJsonDocument<256> doc; //For Json Data Extraction

//////////////////////////////////////////////////\\\\\\\//////\/\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////////////// S E T U P    F U N C T I O N S\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void setup() {
  //Enable flash to be used as a toggle button.
  pinMode(0, INPUT_PULLUP);
  buttonIsPressed = false;
  
  //Enable and turn off built in LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);
  Serial.println("Program Started");
  
  initWifi();
  initLED();
}

void initWifi() {
  //Attempt to connect
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  bool flash = true;
  //Wait for connection
  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    if(flash) {
      digitalWrite(LED_BUILTIN, HIGH);
    }else{
      digitalWrite(LED_BUILTIN, LOW);
    }
    flash = !flash;
    Serial.println("Connecting...");
  }

  //Once connected show LED
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Connected to Wifi!");
  Serial.println(WiFi.localIP());
}

void initLED() { //Enable LED Variables, but turn them off
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  delay(25);
  FastLED.clear(true);
}

//////////////////////////////////////////////////\\\\\\\//////\/\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////////////// S E T U P    F U N C T I O N S    O V E R\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
////////////////////////////////////////////////////////////\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

void loop() {
   //Arbitrary Button Configuration
  if(digitalRead(0) == 0 && !buttonIsPressed) {
    buttonIsPressed = true;
    pressTime = millis();
    deltaTimeButton = 0;
    buttonPress();
  }else if(buttonIsPressed){
    if(digitalRead(0) == 1 && deltaTimeButton > 500) {
      buttonIsPressed = false;
    }
    deltaTimeButton = millis() - pressTime;
  }
/*  
  if(Serial.available()) {
    processSerialInput();
  }  
*/
  doLED();
}

void changeMode(int newMode) {
  const char* username = doc["name"];
  if(ledMode != newMode) {
    ledMode = newMode;
    switch(ledMode) {
      case -1: Serial.print(username);
               Serial.println(" turned off the lights");
               FastLED.clear(true);
               break;
      case 0: setColors();
              initLEDColor();
              break;
      case 1: initFade();
              break;
      case 2: setColors();
              initFlash();
              break;
      default: break;
    }
  }
}

void processSerialInput() {
//  char input[8];
//  Serial.readString().toCharArray(input, 8);
//  //hexStringToInt(input);
}

void doLED() {
  switch(ledMode){
    case 1: moveLED(); //If fade, move leds along. (maybe change this to a less rainbow-y and more gradual fade)
            break;
    case 2: flash();
    default: break; //Other settings do not require continuous updates
  }
}

void setColors() {
  const char* hex = doc["color"];
  char color[5];
  color[0] = '0';
  color[1] = 'x';
  
  color[2] = hex[1];
  color[3] = hex[2];
  r = strtol(color, NULL, 16); //Red
  
  color[2] = hex[3];
  color[3] = hex[4];
  g = strtol(color, NULL, 16); //Green
  
  color[2] = hex[5];
  color[3] = hex[6];
  b = strtol(color, NULL, 16); //Blue
}

void turnOff(int first, int last) {
  for(int i = first; i < last; i++){
    leds[i].setHSV(255, 255, 255);
  }
  FastLED.show();
}

void initLEDColor() {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB(r,g,b);
  }
  FastLED.setBrightness(75);
  FastLED.show();
}

void initFade() { //Probably should rewrite this function, pretty janky algorithm
  FastLED.setBrightness(75); 
  //Set up for fade
  int c = 0;
  for(int b = 0; b < 255; b++){
    leds[b + c].setHue(b);
    if(b % 6 == 0){                        
      c++;
      leds[b + c].setHue(b);
    }
  }
  leds[299].setHue(255);
  leds[298].setHue(255);
  leds[297].setHue(255);
}

void initFlash() {
  isOn = true;
  timer = millis();
  initLEDColor();
}

void moveLED(){
  CRGB temp = leds[299];
  for(int count = NUM_LEDS-1; count>0; count--){
    leds[count] = leds[count-1];
  }
  leds[0] = temp;
  delay(10); // Might need to process this using delta Time? Not sure if this would cause problems with reading from serial input
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
  if(switched) {
    if(isOn) {
      initLEDColor();
    }else {
      FastLED.clear(true);
    }
  }
}

void buttonPress() {
  httpRequest("GET", "http://192.168.1.36:5000/queue/dequeue");
}

void httpRequest(String type, String url) { //Later change the GET request to occur within a while loop with http.connected to simulate a async function();
  Serial.print("Requesting @ " + url + "...");
  http.begin(url);
  
  if(type.equals("GET")) {
    int httpCode = http.GET();
    Serial.println(httpCode);
    Serial.println(http.getString());
    
    if(httpCode == 200) {
      deserializeJson(doc, http.getString());
      const int modeNum = doc["mode"];
      changeMode(modeNum);
    }
  }
  //Dunno if i really need to support post request
  http.end();
}
