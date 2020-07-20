#include <ESP8266WiFi.h>
#include <FastLED.h>

#define WIFI_SSID "f231fa"
#define WIFI_PASS "gain.037.barrier"
#define NUM_LEDS 300
#define LED_PIN 6

CRGB leds[NUM_LEDS];
int mode = 0;
int command;

bool buttonIsPressed;
long pressTime;
long deltaTimeButton = 0;



void setup() {
  //Enable flash to be used as a toggle button.
  pinMode(0, INPUT_PULLUP);
  buttonIsPressed = false;
  
  //Enable and turn off built in LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.begin(115200);
  Serial.println("Program Started");
  
  //initWifi();

  initLED();
}

void initWifi() {
  //Attempt to connect
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  //Wait for connection
  while(WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.println("Connecting...");
  }

  //Once connected show LED
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Connected to Wifi!");
  Serial.println(WiFi.localIP());
}

void initLED() {
  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(100);
  initLEDColor();
}

void loop() {
  /* //Arbitrary Button Configuration
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
  */
  if(Serial.available()) {
    processSerialInput();
  }
  
  doLED();
    
}

void processSerialInput() {
  command = Serial.parseInt();
  
  
  if(mode != command && isValidCommand(command)) {
    Serial.read();
    Serial.println(command);
    mode = command;
    switch(command) {
      case -1: FastLED.clear(true);
      case 0: initLEDColor();
              break;
      case 1: initFade();
    }
  }else {
    Serial.println(command + " cannot be done");
  }
}

bool isValidCommand(int command){
  return command >= -1 && command <= 1;
}

void doLED() {
  switch(mode){
    case 1: moveLED(leds); //If fade, move leds along. (maybe change this to a less rainbow-y and more gradual fade)
            break;
    default: break; //No need to set LEDs to the same color every time or to keep turning them off
  }
}

void moveLED(CRGB leds[NUM_LEDS]){
  CRGB temp = leds[299];
  for(int count = NUM_LEDS-1; count>0; count--){
    leds[count] = leds[count-1];
  }
  leds[0] = temp;
  delay(10);
  FastLED.show();
}

void initLEDColor() {
  FastLED.showColor(CRGB::White);
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

void buttonPress() {
  Serial.println("Button Pressed!");
}
