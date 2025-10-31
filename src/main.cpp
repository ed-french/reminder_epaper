#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// include library, include base class, make path known
#include <GxEPD.h>
// #include "SD.h"
#include "SPI.h"

//! There are three versions of the 2.13 screen,
//  if you are not sure which version, please test each one,
//  if it is successful then it belongs to the model of the file name

//#include <GxGDE0213B1/GxGDE0213B1.h>      // 2.13" b/w
//#include <GxGDEH0213B72/GxGDEH0213B72.h>  // 2.13" b/w new panel
#include <GxGDEH0213B73/GxGDEH0213B73.h>  // 2.13" b/w newer panel


// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/Picopixel.h>
#include <Fonts/FreeSans12pt7b.h>

// #include <Fonts/FreeMono9pt7b.h>
// #include <Fonts/Org_01.h>
// #include <Fonts/Picopixel.h>
// #include <Fonts/Tiny3x3a2pt7b.h>
#include <Fonts/TomThumb.h>



#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
//#include <WiFiClient.h>

#include "credentials.h"

#define TEST_MODE

#define SLEEP_INTERVAL_S 30

#define TIME_CHECK_EVERY_N_UPDATES 0 // Check every time if 0

// // GRAPH CONSTANTS
//   #define X_ORIGIN 145
//   #define Y_ORIGIN 105
//   #define GRAPH_WIDTH 100
//   #define MIN_Y 400.0
//   #define MIN_Y_STR "400"
//   #define MAX_Y 2000.0
//   #define MAX_Y_STR "2000"
//   #define GRAPH_HEIGHT 100
//   #define TICK_WIDTH 5
//   #define CHARACTER_UNIT 5
//   #define LIMIT_VALUE 800



#define SENSOR_SLEEP HIGH
#define SENSOR_AWAKE LOW



#define SPI_MOSI 23
#define SPI_MISO -1
#define SPI_CLK 18

#define ELINK_SS 5
#define ELINK_BUSY 4
#define ELINK_RESET 16
#define ELINK_DC 17

#define SDCARD_SS 13
#define SDCARD_CLK 14
#define SDCARD_MOSI 15
#define SDCARD_MISO 2

#define BUTTON_PIN 39

#define BATTERY_LEVEL_PIN 35
#define LED_PIN 19
#define LED_ON_LEVEL 1
#define LED_OFF_LEVEL 0


GxIO_Class io(SPI, /*CS=5*/ ELINK_SS, /*DC=*/ ELINK_DC, /*RST=*/ ELINK_RESET);
GxEPD_Class display(io, /*RST=*/ ELINK_RESET, /*BUSY=*/ ELINK_BUSY);

//SPIClass sdSPI(VSPI);


WiFiClient client;

// RTC_DATA_ATTR uint16_t dataset[256];
// RTC_DATA_ATTR uint8_t next_dataset_spot=0;
// RTC_DATA_ATTR char last_upload[40];
esp_sleep_wakeup_cause_t wakeup_reason;

// Global time variables
RTC_DATA_ATTR uint8_t hour=0;
RTC_DATA_ATTR uint8_t minute=0;
RTC_DATA_ATTR uint8_t second=0;
// Set time variables (when was the timer set last)
RTC_DATA_ATTR uint8_t settime_hour=0;
RTC_DATA_ATTR uint8_t settime_minute=0;
RTC_DATA_ATTR uint8_t settime_second=0;
// Set timer variables
RTC_DATA_ATTR uint8_t timer_hours=0;
RTC_DATA_ATTR uint8_t timer_minutes=0;
RTC_DATA_ATTR uint8_t timer_seconds=0;



RTC_DATA_ATTR uint16_t update_count=0;


// uint8_t sensor_to_y_px(float value)
// {
//   return (uint8_t)(Y_ORIGIN-((value-MIN_Y)*GRAPH_HEIGHT/(MAX_Y-MIN_Y)));
// }

esp_sleep_wakeup_cause_t get_wakeup_reason(){
	wakeup_reason = esp_sleep_get_wakeup_cause();
	switch(wakeup_reason)
	{
		case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
		case ESP_SLEEP_WAKEUP_EXT1  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
		case ESP_SLEEP_WAKEUP_TIMER  : Serial.println("Wakeup caused by timer"); break;
		case ESP_SLEEP_WAKEUP_TOUCHPAD  : Serial.println("Wakeup caused by touchpad"); break;
		case ESP_SLEEP_WAKEUP_UNDEFINED  : Serial.println("Wakeup caused by UNDEFINED"); break;
    case ESP_SLEEP_WAKEUP_ALL:  Serial.println("Wakeup caused by ALL"); break;
		default : Serial.println("Wakeup was not caused by deep sleep"); break;
	}
  return wakeup_reason;
}



bool setTimesFromServer(String url)
{
      // Connects to the url and returns the string there
      // a string with three hh:mm:ss values seperated by newlines  
      // e.g. 20:28:48 20:28:38 20:38:38 
      //      01:34:67 90:23:56 89:12:45
      //      current  set      timer
      // first is current time, second is set time, third is timer time
      if (WiFi.status()==WL_CONNECTED)
      {
        HTTPClient http;
        http.begin(url);
        int httpResponseCode = http.GET();
        if (httpResponseCode>0)
        {
          String payload = http.getString();
          Serial.println("Time from server: "+payload);
          
          // Current time
          hour=payload.substring(0,2).toInt();
          minute=payload.substring(3,5).toInt();
          second=payload.substring(6,8).toInt();

          // settime
          settime_hour=payload.substring(9,11).toInt();
          settime_minute=payload.substring(12,14).toInt();
          settime_second=payload.substring(15,17).toInt();

          // timer time
          timer_hours=payload.substring(18,20).toInt();
          timer_minutes=payload.substring(21,23).toInt(); 
          timer_seconds=payload.substring(24,26).toInt();


          Serial.printf("Parsed time now: %02d:%02d:%02d\n",hour,minute,second);
          http.end();
          
          return true;
        }
        else
        {
          Serial.printf("Error in HTTP request, code: %d\n",httpResponseCode);
        }
        http.end();
        return false;
      }
      else
      {
        Serial.println("Not connected to WiFi");
      }
      return false;
     
}

void drawScreen()
{
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  
    display.setFont(&FreeMonoBold9pt7b);
  //display.setCursor(180,121);
  display.setCursor(display.width()-60,12);
  // display.setTextSize(2);
  display.printf("%.2fv ,",3.3*analogRead(BATTERY_LEVEL_PIN)/2048);
  // display.setTextSize(1);
  // display.printf("Last upload: %s",last_upload);
  // display.setFont(&FreeMonoBold12pt7b);
  display.setFont(&FreeSans12pt7b);
  
  display.setCursor(0,18);
  // display.print("Time:");
  // display.setFont(&FreeMonoBold24pt7b);
  // display.setCursor(0,60);
  // display.printf("%02d:%02d:%02d",hour,minute,second);
  display.printf("Now       %02d:%02d\nTime up %02d:%02d\n",hour,minute,timer_hours,timer_minutes);
  // Calculate time remaining
  int32_t total_seconds_now=hour*3600+minute*60+second;
  int32_t total_seconds_set=settime_hour*3600+settime_minute*60+settime_second;
  int32_t total_seconds_timer=timer_hours*3600+timer_minutes*60+timer_seconds;
  int32_t total_seconds_left=total_seconds_timer-total_seconds_now;
  if (total_seconds_left>0)
  {
    display.printf("Left   %02d:%02d:%02d",total_seconds_left/3600,(total_seconds_left%3600)/60,(total_seconds_left%3600)%60);
  }
  display.setCursor(0, display.height() - 35);
  display.drawRect(0, display.height() - 35, display.width(), 35, GxEPD_BLACK);
  if (total_seconds_left>0)
  {
    float proportion_left=(float)total_seconds_left/(float)(total_seconds_timer-total_seconds_set);
    uint16_t rect_width=(uint16_t)(proportion_left*(float)display.width());
    Serial.printf("Proportion left: %f.0d\nWidth: %d\n",proportion_left,rect_width);
    display.fillRect(0, display.height() - 35, rect_width, 35, GxEPD_BLACK);
  }

  display.update();
}


// void drawGraph()
// {
//   // Draws graph with data in on right hand side of screen




//   uint8_t threshold_y=sensor_to_y_px(LIMIT_VALUE);
//   //Warn Zone
//   display.fillRect(X_ORIGIN,threshold_y,GRAPH_WIDTH,Y_ORIGIN-threshold_y,GxEPD_LIGHTGREY);


//   display.drawFastHLine(X_ORIGIN,Y_ORIGIN,GRAPH_WIDTH,GxEPD_BLACK);
//   display.drawFastVLine(X_ORIGIN,Y_ORIGIN-GRAPH_HEIGHT,GRAPH_HEIGHT,GxEPD_BLACK);
  

//   //y-axis labels
//   display.setCursor(X_ORIGIN-4*CHARACTER_UNIT,Y_ORIGIN+CHARACTER_UNIT-5);
//   display.setFont(&TomThumb);
//   display.setTextSize(1);
//   display.print(MIN_Y_STR);
//   display.setCursor(X_ORIGIN-5*CHARACTER_UNIT,Y_ORIGIN-GRAPH_HEIGHT+CHARACTER_UNIT-2);
//   display.print(MAX_Y_STR);

//   //y-axis ticks
//   display.drawFastHLine(X_ORIGIN-TICK_WIDTH,Y_ORIGIN,TICK_WIDTH,GxEPD_BLACK);
//   display.drawFastHLine(X_ORIGIN-TICK_WIDTH,Y_ORIGIN-GRAPH_HEIGHT,TICK_WIDTH,GxEPD_BLACK);




//   //limit line
//     for (uint8_t x=X_ORIGIN;x<X_ORIGIN+GRAPH_WIDTH;x+=5)
//   {
//     display.drawFastHLine(x,threshold_y,3,GxEPD_BLACK);
//   }
  
//   for (uint8_t x=0;x<GRAPH_WIDTH;x++)
//   {
//     uint8_t bar_height;
//     uint8_t bar_top;

//     uint8_t index=next_dataset_spot-GRAPH_WIDTH+x;
//     uint16_t data=dataset[index];
//     //Serial.println("Graph points:");
//     if (data>0)
//     {
      
//       bar_height=(uint8_t)((data-MIN_Y)*GRAPH_HEIGHT/(MAX_Y-MIN_Y));
//       bar_top=Y_ORIGIN-bar_height;
//       Serial.printf("%4d,%4d,%4d,%4d\n",x,data,bar_height,bar_top);
//       display.drawFastVLine(X_ORIGIN+x,bar_top,bar_height,GxEPD_BLACK);
//     }
    
    
//   }


// }


// float getSensorReading()
// {

//   uint16_t random_co2=(random(0,1600));
//   Serial.printf("Simulated CO2: %d ppm\n",random_co2);
//   return (float)random_co2;
// }


void setup() {
  pinMode(LED_PIN,OUTPUT);
  // digitalWrite(LED_PIN,LED_ON_LEVEL);

  // pinMode(PIN_SENSOR_WAKE,OUTPUT);
  // digitalWrite(PIN_SENSOR_WAKE,SENSOR_AWAKE);




  Serial.begin(115200);
  delay(2000);
  // Serial.printf("Readings next spot: %d\n",next_dataset_spot);

  if (get_wakeup_reason()!=ESP_SLEEP_WAKEUP_TIMER)
  {
    Serial.println("Non timer wakeup");
    update_count=0;

  }

    // Initialize SPI bus and ensure e-link pins are GPIO outputs
  SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, ELINK_SS);
  pinMode(ELINK_SS, OUTPUT);
  pinMode(ELINK_DC, OUTPUT);
  pinMode(ELINK_RESET, OUTPUT);
  

  if (update_count==0)
  {
    WiFi.begin(ssid, password);
    uint16_t trycount=0;
    bool connected=false;
    while (WiFi.status() != WL_CONNECTED && trycount<20) {
    delay(1000);
    trycount++;
    Serial.println("Connecting to WiFi..");
    }
    if (WiFi.status()==WL_CONNECTED) connected=true;
  
    Serial.printf("%s to wifi network\n",connected ? "Connected" : "Not-Connected");

    // digitalWrite(LED_PIN,LED_ON_LEVEL);

    setTimesFromServer("http://192.168.1.125/timer_status");
    update_count=1;
  } else {
    Serial.println("Skipping time update this cycle");
    update_count++;
    if (update_count>TIME_CHECK_EVERY_N_UPDATES){
      update_count=0;
      Serial.println("Resetting update count to zero for time update next cycle");
    } 
    // Estimate time progression
    second+=SLEEP_INTERVAL_S+11; // add 11 seconds to cover delays (actually 11.05s average)
    if (second>=60){
      second-=60;
      minute++;
      if (minute>=60){
        minute=0;
        hour++;
        if (hour>=24){
          hour=0;
        }
      }
    }


  }
  Serial.printf("Current time: %02d:%02d:%02d\n",hour,minute,second);

  // SPI.begin(SPI_CLK, SPI_MISO, SPI_MOSI, ELINK_SS);
  display.init(); // enable diagnostic output on Serial

  display.setRotation(3);
  // //display.fillScreen(GxEPD_WHITE);//
  // //display.drawRect(0,display.height()-15,display.width(),30,GxEPD_WHITE);
  // display.setTextColor(GxEPD_BLACK);


// #include <Fonts/FreeMono9pt7b.h>
// #include <Fonts/Org_01.h>
// #include <Fonts/Picopixel.h>
// #include <Fonts/Tiny3x3a2pt7b.h>
// #include <Fonts/TomThumb.h>
  // display.setCursor(0,10);
  // display.setFont(&FreeMono9pt7b);
  // display.print("FreeMono9pt7b");

  // display.setCursor(0,10);
  // display.setFont(&Org_01);
  // display.print("Org_01");

  // display.setCursor(0,20);
  // display.setFont(&Picopixel);
  // display.print("Picopixel");

  // display.setCursor(0,30);
  // display.setFont(&Tiny3x3a2pt7b);
  // display.print("Tiny3x3..");

  // display.setCursor(0,40);
  // display.setFont(&TomThumb);
  // display.print("TomThumb");






  // display.update();
  // delay(10000);










  // display.setFont(&FreeMonoBold12pt7b);
  // display.setCursor(0,40);
  // display.print("CO2 ppm");

  // display.setCursor(0,80);
  // display.setFont(&FreeMonoBold24pt7b);
  // display.printf("%.0f",reading);
  Serial.println("Drawing screen");
  drawScreen();
  Serial.println("Screen drawn");
  //
  display.update();
  Serial.println("Display updated");


  // uint64_t sleeptime=connected ? (SLEEP_INTERVAL_MINS*60000000ull) : (SLEEP_INTERVAL_MINS*60000000ull); // 3 second regardless
  uint64_t sleeptime=(SLEEP_INTERVAL_S*1000000ull); // 
  Serial.printf("Sleeping for %d seconds\n",(uint32_t)(sleeptime/1000000));


  esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, LOW);
  esp_sleep_enable_timer_wakeup(sleeptime); // 300 million MICROSECONDS= 5 mins!
 
  // digitalWrite(LED_PIN,LED_ON_LEVEL);

  // digitalWrite(LED_PIN,LED_OFF_LEVEL);
  // digitalWrite(PIN_SENSOR_WAKE,SENSOR_SLEEP);
  delay(100);
  // pinMode(PIN_SENSOR_WAKE,INPUT); // Go high impedance
  float time_taken=(float)millis()/1000.0;
  Serial.printf("Took %.2f + 1 seconds to run\n",time_taken);
  delay(1000);
  // digitalWrite(LED_PIN,LED_OFF_LEVEL);
  esp_deep_sleep_start();






}

void loop() {
  // put your main code here, to run repeatedly:
}