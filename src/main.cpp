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

#define NUMBER_PAIR_HORIZONTAL_WIDTH 25
#define NUMBER_PAIR_VERTICAL_PITCH 22
#define SPACING_BETWEEN_NUMBER_PAIRS 6
#define NUMBER_OFFSET_TOP 16
#define HOUR_LEFT_OFFSET 60

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

char time_build_buffer[20];

RTC_DATA_ATTR uint8_t last_time_elements[12]; // To store last time elements to detect time changes


RTC_DATA_ATTR uint8_t progress_pixels=255; // To store last time elements to detect time changes



class DisplayableTime
{
  public:
    DisplayableTime(uint8_t x, uint8_t y, bool show_seconds, const char* lbl, uint8_t store_index )
    {
      left=x;
      top=y;
      show_secs=show_seconds;
      hour=0;
      minute=0;
      second=0;
      storage_index=store_index;
      // Retreive last times from storage
      hour=last_time_elements[storage_index*3+0];
      
      minute=last_time_elements[storage_index*3+1];
      second=last_time_elements[storage_index*3+2];
      Serial.printf("Initialized DisplayableTime %s with last time %02d:%02d:%02d\n",lbl,hour,minute,second);
      strncpy(label,lbl,sizeof(label));
      label[sizeof(label)-1]='\0'; // Surely unncessary but just in case
    }

    void retreiveOldTimes()
    {

      hour=last_time_elements[storage_index*3+0];
      minute=last_time_elements[storage_index*3+1];
      second=last_time_elements[storage_index*3+2];

      Serial.printf("Retreived DisplayableTime %s with last time %02d:%02d:%02d\n",label,hour,minute,second);

    }

    void setTime(uint8_t h, uint8_t m, uint8_t s=0)
    {
      // Once we have retreived a new time from the server, we need to
      // 1. Compare with the stored values to see what changed
      hour_changed= (h!=hour);
      minute_changed= (m!=minute);
      second_changed= (s!=second);
      // 2. Store the new values
      last_time_elements[storage_index*3+0]=h;
      last_time_elements[storage_index*3+1]=m;
      last_time_elements[storage_index*3+2]=s;
      Serial.printf("%s change %d:%d:%d)\n",label,hour_changed,minute_changed,second_changed);
      hour=h;
      minute=m;
      second=s;
    }

    void toString(char * buffer)
    {
      sprintf(buffer,"%02d:%02d:%02d",hour,minute,second);
    }

    void display(GxEPD_Class &disp, bool full_redraw)
    {
      // disp.eraseDisplay(true);
      disp.setFont(&FreeSans12pt7b);
      disp.setTextColor(GxEPD_BLACK);
      disp.setTextSize(1);
      if (full_redraw)
      {
        disp.setCursor(0,top);
        disp.print(label);
      }

      if (hour_changed || full_redraw)
      {
        Serial.printf("About to print hour for %s at: %d,%d\n",label,left,top);
        _update_number(hour,left,top,disp);
      }

      if (minute_changed || full_redraw)
      {
        _update_number(minute,left+NUMBER_PAIR_HORIZONTAL_WIDTH+SPACING_BETWEEN_NUMBER_PAIRS,top,disp);

      }
      if (show_secs && (second_changed || full_redraw))
      {
        _update_number(second,left+2*(NUMBER_PAIR_HORIZONTAL_WIDTH+SPACING_BETWEEN_NUMBER_PAIRS),top,disp);
      }
    }

    uint32_t totalSeconds()
    {
      return hour*3600+minute*60+second;
    }

    void _update_number(uint8_t value,int8_t x, uint8_t y, GxEPD_Class &disp)
    {
      disp.setFont(&FreeSans12pt7b);
      disp.setTextColor(GxEPD_BLACK);
      disp.setTextSize(1);
      char new_text[4];
      sprintf(new_text,"%02d",value);
      uint8_t max_width=NUMBER_PAIR_HORIZONTAL_WIDTH;

      // Clear the area
      disp.fillRect(x-1,y-16,max_width+2,NUMBER_PAIR_VERTICAL_PITCH-2,GxEPD_WHITE);
      // Draw the new text
      disp.setCursor(x,y);
      disp.print(new_text);
      disp.updateWindow(x-1,y-16,max_width+2,NUMBER_PAIR_VERTICAL_PITCH-2,true);
      // disp.update();
      
    }


  private:
    uint8_t left;
    uint8_t top;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    bool hour_changed;
    bool minute_changed;
    bool second_changed;
    bool show_secs;
    char label[30];
    uint8_t storage_index;
  

};

DisplayableTime currentTime(HOUR_LEFT_OFFSET,NUMBER_OFFSET_TOP,false,"Now",0);
DisplayableTime setTime(HOUR_LEFT_OFFSET,255,false,"When set",1); // Won't normally be displayed- just using this class for consistency
DisplayableTime timerTime(HOUR_LEFT_OFFSET,NUMBER_OFFSET_TOP+NUMBER_PAIR_VERTICAL_PITCH,false,"Due",2);
DisplayableTime timeLeft(HOUR_LEFT_OFFSET,NUMBER_OFFSET_TOP+NUMBER_PAIR_VERTICAL_PITCH*2,true,"Left",3);

RTC_DATA_ATTR uint16_t update_count=0;



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

void sleep_now(uint32_t sleep_seconds)
{
  uint64_t sleeptime=(sleep_seconds*1000000ull); // 
  Serial.printf("Going to sleep for %d seconds\n",sleep_seconds);
  esp_sleep_enable_timer_wakeup(sleeptime); // 300 million MICROSECONDS= 5 mins!
  delay(500);
  esp_deep_sleep_start();
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
        http.setTimeout(10000); // 10 second timeout
        http.begin(url);
        int httpResponseCode = http.GET();
        if (httpResponseCode>0)
        {
          String payload = http.getString();
          Serial.println("Time from server: "+payload);
          
          // Current time
          uint8_t hour=payload.substring(0,2).toInt();
          uint8_t minute=payload.substring(3,5).toInt();
          uint8_t second=payload.substring(6,8).toInt();

          currentTime.setTime(hour,minute,second);

          // settime
          uint8_t settime_hour=payload.substring(9,11).toInt();
          uint8_t settime_minute=payload.substring(12,14).toInt();
          uint8_t settime_second=payload.substring(15,17).toInt();
          setTime.setTime(settime_hour,settime_minute,settime_second);

          // timer time
          uint8_t timer_hours=payload.substring(18,20).toInt();
          uint8_t timer_minutes=payload.substring(21,23).toInt(); 
          uint8_t timer_seconds=payload.substring(24,26).toInt();
          timerTime.setTime(timer_hours,timer_minutes,timer_seconds);


          Serial.printf("Parsed time now: %02d:%02d:%02d\n",hour,minute,second);
          http.end();
          
          return true;
        }
        else
        {
          Serial.printf("Error in HTTP request, code: %d\n",httpResponseCode);
          Serial.println("Problem fetching times from server so restarting");
          Serial.println("WiFi status: "+String(WiFi.status()));
          sleep_now(1); // Sleep for 1 second (restarting looses RTC memory)
        }
        http.end();
        return false;
      }
      else
      {
        Serial.println("Not connected to WiFi");
        Serial.println("WiFi status: "+String(WiFi.status()));
        sleep_now(1); // Sleep for 1 second (restarting looses RTC memory)
      }
      return false;
     
}


void drawScreen(bool use_full_update)
{
  if (use_full_update) display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  

  // Show battery level
  
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(display.width()-60,12);
  display.printf("%.2fv",3.3*analogRead(BATTERY_LEVEL_PIN)/2048);

  currentTime.display(display,use_full_update);
  timerTime.display(display,use_full_update);


  int32_t total_seconds_now=currentTime.totalSeconds();
  int32_t total_seconds_when_time_set=setTime.totalSeconds();
  int32_t total_seconds_when_run_out=timerTime.totalSeconds();
  int32_t total_seconds_left=total_seconds_when_run_out-total_seconds_now;
  char now_time_buffer[20];
  currentTime.toString(now_time_buffer);
  char end_time_buffer[20];
  timerTime.toString(end_time_buffer);
  Serial.printf("Total seconds left: %d (i.e. %s - %s) or (%d - %d)\n",total_seconds_left,end_time_buffer,now_time_buffer,total_seconds_when_run_out,total_seconds_now);
  if (total_seconds_left>0)
  {
    timeLeft.setTime((uint8_t)(total_seconds_left/3600),(uint8_t)((total_seconds_left%3600)/60),(uint8_t)((total_seconds_left%3600)%60));
    timeLeft.display(display,use_full_update);
  }

  if (use_full_update)
  {
    display.setCursor(0, display.height() - 35);
    display.drawRect(0, display.height() - 35, display.width(), 35, GxEPD_BLACK);
  }

  if (total_seconds_left>0)
  {
    float proportion_left=(float)total_seconds_left/(float)(total_seconds_when_run_out-total_seconds_when_time_set);
    uint16_t rect_width=(uint16_t)(proportion_left*(float)display.width());
    // Three options at this point:
    // 1. Full update - just redraw the whole rectangle
    // 2a. Changed but - just draw the extra bit
    // 3. It's the same size as last time, so nothing required//    Serial.printf("Proportion left: %.3f\nWidth: %d\n",proportion_left,rect_width);
      
    if (use_full_update)
    {
      display.fillRect(0, display.height() - 35, rect_width, 35, GxEPD_BLACK);
    } else {
      if (rect_width>progress_pixels) // extend the black rectangle
      {
        uint8_t extra_width=rect_width-progress_pixels;

        // TEMP SWITCHED OFF PARTIAL UPDATES FOR TESTING
        // display.fillRect(progress_pixels, display.height() - 35, rect_width-progress_pixels, 35, GxEPD_BLACK);
        // display.updateWindow(progress_pixels, display.height() - 35, extra_width, 35, true);
      } 
      
      progress_pixels=rect_width;
    }
    
    Serial.printf("Proportion left: %.3f\nWidth: %d\n",proportion_left,rect_width);
    // display.fillRect(0, display.height() - 35, rect_width, 35, GxEPD_BLACK);
  }

  if (use_full_update) display.update();
}




void setup() {
  pinMode(LED_PIN,OUTPUT);
  // digitalWrite(LED_PIN,LED_ON_LEVEL);

  // pinMode(PIN_SENSOR_WAKE,OUTPUT);
  // digitalWrite(PIN_SENSOR_WAKE,SENSOR_AWAKE)

  update_count++;




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
  

  currentTime.retreiveOldTimes();
  setTime.retreiveOldTimes();   
  timerTime.retreiveOldTimes();
  timeLeft.retreiveOldTimes();


  WiFi.begin(ssid, password);
  uint16_t trycount=0;
  bool connected=false;
  while (WiFi.status() != WL_CONNECTED && trycount<20)
  {
    delay(1000);
    trycount++;
    Serial.println("Connecting to WiFi..");
  }
  if (WiFi.status()==WL_CONNECTED)
  {
    connected=true;
  } else {
    
    
    // Restart as we don't work without wifi
    sleep_now(1); // Sleep for 1 second (restarting looses RTC memory)
  }

  Serial.printf("%s to wifi network\n",connected ? "Connected" : "Not-Connected");

  // digitalWrite(LED_PIN,LED_ON_LEVEL);

  

  display.init(115200); // enable diagnostic output on Serial
  display.setRotation(3);
  

  //
  // display.update();
  // Serial.println("Display updated");


  // uint64_t sleeptime=(SLEEP_INTERVAL_S*1000000ull); // 
  // Serial.printf("Sleeping for %d seconds\n",(uint32_t)(sleeptime/1000000));


  // esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON_PIN, LOW);
  // esp_sleep_enable_timer_wakeup(sleeptime); // 300 million MICROSECONDS= 5 mins!
 
  // // digitalWrite(LED_PIN,LED_ON_LEVEL);

  // // digitalWrite(LED_PIN,LED_OFF_LEVEL);
  // // digitalWrite(PIN_SENSOR_WAKE,SENSOR_SLEEP);
  // delay(100);
  // // pinMode(PIN_SENSOR_WAKE,INPUT); // Go high impedance
  // float time_taken=(float)millis()/1000.0;
  // Serial.printf("Took %.2f + 1 seconds to run\n",time_taken);
  // delay(1000);
  // // digitalWrite(LED_PIN,LED_OFF_LEVEL);
  // esp_deep_sleep_start();






}


void loop() {
  // put your main code here, to run repeatedly:
  setTimesFromServer("http://192.168.1.125/timer_status");

  currentTime.toString(time_build_buffer);
  Serial.printf("Current time: %s\n",time_build_buffer);
  bool update_full=(update_count%7==0); // Full update every 7 updates);
  

  Serial.printf("Display size: %d x %d\n",display.width(),display.height());

  

  // display.printf("%.0f",reading);
  Serial.println("Drawing screen");
  
  drawScreen(update_full);
  Serial.println("Screen drawn");
  update_count++;
  delay(5000);
}