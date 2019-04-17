// WiFi Includes
#include <ESP8266WiFi.h>
#include <WifiUDP.h>
// WiFi Settings

#ifndef STASSID
#define STASSID "IoT"
#define STAPSK  "4g7TqE4upLpQGsrd"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;

// -----------------------------------------------------------------
// LED stuff
#include <FastLED.h>
#define NUM_LEDS 60
#define mirror_hands   false
bool run_once = false;
uint8_t avgLight;

CRGBArray<NUM_LEDS> leds;

// Colors
CRGB ColorHour    = CRGB(  0,   0, 255);
CRGB ColorMinute  = CRGB(  0, 255,   0);
CRGB ColorCombine = CRGB(  0, 255, 255);
CRGB ColorSecond  = CRGB(255, 255, 255);
CRGB ColorBlack   = CRGB(  0,   0,   0);

// -----------------------------------------------------------------
// NTP / Time Includes
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>

// NTP Settings
#define NTP_ADDRESS "north-america.pool.ntp.org"
#define NTP_OFFSET   60 * 60      // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds

// Set up the NTP UDP client and time variables
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);


// -----------------------------------------------------------------
// Some more #defines
// Predefined animation speeds
#define SLOW 250
#define MEDIUM 50
#define FAST 5

// Various variables ;)
byte hour_hand, minute_hand, second_hand, previous_second;
time_t local, utc;
byte ledBrightness;
bool doDebug = true;


// -----------------------------------------------------------------
// The required Setup function
// -----------------------------------------------------------------
void setup() {
  // Initialize LEDs
  Serial.begin(115200);
  Serial.println("");
  Serial.println("************************");
  Serial.println("DIY ESP & NeoPixel Clock");
  Serial.println("************************");
  FastLED.addLeds<NEOPIXEL,2>(leds, NUM_LEDS);
 
  // Set up and connect to the WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
  }
  paintItBlack();
}

// -----------------------------------------------------------------
// Loop
// -----------------------------------------------------------------
void loop () {
  timeClient.update();
  unsigned long epochTime =  timeClient.getEpochTime();

  // convert received time stamp to time_t object
  time_t local, utc;
  utc = epochTime;

  // Then convert the UTC UNIX timestamp to local time
  TimeChangeRule usEDT = {"EDT", Second, Sun, Mar, 2, -300};  //UTC - 5 hours - change this as needed
  TimeChangeRule usEST = {"EST", First, Sun, Nov, 2, -360};   //UTC - 6 hours - change this as needed
  Timezone usEastern(usEDT, usEST);
  local = usEastern.toLocal(utc);
  byte hour_offset;
  // Need to add the code to brighten or dim clock based on time of day
  adjustBrightness();
  
  minute_hand=minute(local);
  if (minute_hand>=10) {
    hour_offset=(minute_hand / 10)-1;
  }else
  {
    hour_offset=0;
  }
  
  if (hour(local) >= 12) {
    hour_hand = ((hour(local) - 12) * 5) + hour_offset;
  }
  else {
    hour_hand = (hour(local) * 5) + hour_offset;
  }
   
  if (mirror_hands) {
    hour_hand=60-hour_hand;
    minute_hand=60-minute_hand;
    second_hand=(60-second(local));
    if (second_hand==60) {
      second_hand=0;
    }
    if (minute_hand==60) {
      minute_hand=0;
    }
    if (hour_hand==60) {
      hour_hand=0;
    }
  } else {
    second_hand=second(local);
  }
  
  if (second_hand!=previous_second) {
    previous_second=second_hand;
    drawHands();
  }
  if (doDebug) {
    Serial.printf("hour:%d (%d), minute:%d second:%d (%d) \n",hour(local),hour_hand,minute_hand,second_hand);
  }
  
}

// -----------------------------------------------------------------
// Functions Start Here
// -----------------------------------------------------------------
// Function to set all LEDs to black
// -----------------------------------------------------------------
void paintItBlack () {
  for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
  }
  FastLED.show();
}

// -----------------------------------------------------------------
// Quick animation for 15/45 minutes
// -----------------------------------------------------------------
void quarterHourAnimation(){
  while (run_once != true) {
    static uint8_t hue;
    for(int i = 0; i < NUM_LEDS/2; i++) {   
      // fade everything out
      leds.fadeToBlackBy(40);

      // let's set an led value
      leds[i] = CHSV(hue++,255,255);

      // now, let's first 20 leds to the top 20 leds, 
      leds(NUM_LEDS/2,NUM_LEDS-1) = leds(NUM_LEDS/2 - 1 ,0);
      FastLED.delay(30);
      avgLight = leds[NUM_LEDS/2].getAverageLight();
    }
    // Fade the rest of the LEDs out
    while (avgLight != 0) {
      avgLight = leds[NUM_LEDS/2].getAverageLight();
      leds.fadeToBlackBy(40);
      FastLED.delay(30);
    }
    // Set any stragglers to black (off)
    paintItBlack();
    run_once = true;
  }
  run_once = false;
}

// -----------------------------------------------------------------
// TheaterChase Animation
// -----------------------------------------------------------------
void theaterChase(CRGB c, int cycles, int speed){
  for (int j=0; j<cycles; j++) {  
    for (int q=0; q < 3; q++) {
      for (int i=0; i < NUM_LEDS; i=i+3) {
        int pos = i+q;
        leds[pos] = c;    //turn every third pixel on
      }
      FastLED.show();

      FastLED.delay(speed);

      for (int i=0; i < NUM_LEDS; i=i+3) {
        leds[i+q] = CRGB::Black;        //turn every third pixel off
      }
    }
  }
}

// -----------------------------------------------------------------
// Functions for getting random colors
// -----------------------------------------------------------------
CRGB Wheel(byte WheelPos) {
  if(WheelPos < 85) {
    return CRGB(WheelPos * 3, 255 - WheelPos * 3, 0);
  } 
  else if(WheelPos < 170) {
    WheelPos -= 85;
    return CRGB(255 - WheelPos * 3, 0, WheelPos * 3);
  } 
  else {
    WheelPos -= 170;
    return CRGB(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}

CRGB randomColor(){
  return Wheel(random(256)); 
}


// -----------------------------------------------------------------
// Draw the hands!
// -----------------------------------------------------------------
void drawHands() {
  paintItBlack();
  
  // Do something on the hour
  if ((minute_hand == 0) && (second_hand == 0)) {
    for(int i=0; i<3; i++){
      theaterChase(randomColor(),10,MEDIUM);
    }
  }

  // Do something every 15 mintues;
  else if ( ((minute_hand % 15 ) == 0) && (minute_hand != 0) && (second_hand == 0) ) {
    quarterHourAnimation();
    quarterHourAnimation();
  }
  
  // Resume normal function
//  pixels.setPixelColor(hour_hand, ColorHour );
  leds[hour_hand] = CRGB(ColorHour);
  // if hour and minute are the same led, use a different color to show that
  if (hour_hand==minute_hand) {
    leds[ minute_hand ] = CRGB( ColorCombine );
  } else {
    leds[ minute_hand ] = CRGB( ColorMinute );
  }

  // draw the second LED, using medium brightness white
  leds[ second_hand ] = CRGB( ColorSecond );

  // show all the LED's, only the ones we have set with a color will be visible.
  FastLED.show();
}

void adjustBrightness () {
  if ( (24 >= hour(local) >= 18) or (0 <= hour(local) <= 6) ) {
    ledBrightness = 75;
//    FastLED.setBrightness(75);
//    Serial.println("Brightness set to 75");
  } else {
    ledBrightness = 200;
//    FastLED.setBrightness(200);
//    Serial.println("Brightness set to 200");
  }
  FastLED.setBrightness(ledBrightness);
  if (doDebug) {
    Serial.printf("Brightness set to %d\n", ledBrightness);
  }
}
