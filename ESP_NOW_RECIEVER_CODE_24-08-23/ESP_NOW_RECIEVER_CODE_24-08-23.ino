//Receiver Sketch
//-----------------------------------------------------------
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <AsyncTCP.h>
#include <FS.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <Arduino_JSON.h>
#include <LiquidCrystal_I2C.h>
#include <HTTPClient.h>
#include "time.h"
#include "ESP32_MailClient.h"
//-----------------------------------------------------------
JSONVar board;
AsyncWebServer server(80);
AsyncEventSource events("/events");

//TaskHandle_t Task1;

//=======================================EMAIL CREDENTIALS===========================================

#define emailSenderAccount    "esp.mail.010@gmail.com"
#define emailSenderPassword   "edwkoqjlsjcgaovf"
#define smtpServer            "smtp.gmail.com"
#define smtpServerPort        465
#define emailSubject          "[ALERT]"

// Default Recipient Email Address
String inputMessage = "majumder.ritam.02@gmail.com";

// The Email Sending data object contains config and data to send
SMTPData smtpData;
//===================================================================================================

//==============================DECLARING FUNCTIONS FOR LCD DISPLAY==================================
LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address to 0x3F for a 16 chars and 2 line display

//===================================================================================================
#define led 2
int ledState = LOW;   // ledState used to set the LED
unsigned long previousMillis = 0;   // will store last time LED was updated

//---------------------------------------------------------------------------------------------------
float temp_val_1 = 0.0;
float hum_val_1 = 0.0;
float heatI_1 = 0.0;
float mq4_1 = 0.0;
float mq7_1 = 0.0;
float mq9_1 = 0.0;
float mq135_1 = 0.0;
float PM_1 = 0.0;

float temp_val_2 = 0.0;
float hum_val_2 = 0.0;
float heatI_2 = 0.0;
float mq4_2 = 0.0;
float mq7_2 = 0.0;
float mq9_2 = 0.0;
float mq135_2 = 0.0;
float PM_2 = 0.0;

float temp_val_3 = 0.0;
float hum_val_3 = 0.0;
float heatI_3 = 0.0;
float mq4_3 = 0.0;
float mq7_3 = 0.0;
float mq9_3 = 0.0;
float mq135_3 = 0.0;
float PM_3 = 0.0;

float temp_avg = 0.0;
float hum_avg = 0.0;
float heatI_avg = 0.0;
float mq4_avg = 0.0;
float mq7_avg = 0.0;
float mq9_avg = 0.0;
float mq135_avg = 0.0;
float PM_avg = 0.0;

String loc1_status = "SAFE";
String loc2_status = "SAFE";
String loc3_status = "SAFE";
//---------------------------------------------------------------------------------------------------

//---------------------------------------------------------------------------------------------------
// Replace with your network credentials (STATION)
//const char* ssid = "TP-Link_907E";
//const char* password = "ritam@09";

const char* ssid = "Ritam";
const char* password = "ritam0909";
//---------------------------------------------------------------------------------------------------

//==================================ESP NOW RECEIVER STRUCTURE=======================================

typedef struct RxStruct
{
    int ID;
    float dht_temp;
    float dht_humid;
    float heat_index;
    float mq4_val;
    float mq7_val;
    float mq9_val;
    float mq135_val;
    float PM_val;
}RxStruct;

// Create a struct_message called myData
RxStruct receivedData;

// Create a structure to hold the readings from each board
RxStruct board1;
RxStruct board2;
RxStruct board3;

// Create an array with all the structures
RxStruct boardsStruct[3] = {board1, board2, board3};

//===================================================================================================

//=================================ESP NOW RECEIVER FUNCTION=========================================

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
{
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  blinkit();
  
  // Update the structures with the new incoming data
  boardsStruct[receivedData.ID-1].dht_temp = receivedData.dht_temp;
  boardsStruct[receivedData.ID-1].dht_humid = receivedData.dht_humid;
  boardsStruct[receivedData.ID-1].heat_index = receivedData.heat_index;
  boardsStruct[receivedData.ID-1].mq4_val = receivedData.mq4_val;
  boardsStruct[receivedData.ID-1].mq7_val = receivedData.mq7_val;
  boardsStruct[receivedData.ID-1].mq9_val = receivedData.mq9_val;
  boardsStruct[receivedData.ID-1].mq135_val = receivedData.mq135_val;
  boardsStruct[receivedData.ID-1].PM_val = receivedData.PM_val;
}

//===================================================================================================

//==================================GOOGLE SHEET CREDENTIALS========================================
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 0;
// Google script ID and required credentials
String GOOGLE_SCRIPT_ID = "AKfycbwZJpJkHUZ1Z1SeDoIgAARQlF32AIaZ-3j0RrM8W8Mr2TSFJxyVSR3BxEkmXRT2tChl1g";

//===================================================================================================

//====================================FUNCTIONS TO SEND EMAIL========================================

bool sendEmailNotification(String emailMessage)
{
  // Set the SMTP Server Email host, port, account and password
  smtpData.setLogin(smtpServer, smtpServerPort, emailSenderAccount, emailSenderPassword);

  // Set the sender name and Email
  smtpData.setSender("ESP32", emailSenderAccount);

  // Set Email priority or importance High, Normal, Low or 1 to 5 (1 is highest)
  smtpData.setPriority("Highest");

  // Set the subject
  smtpData.setSubject(emailSubject);

  // Set the message with HTML format
  smtpData.setMessage(emailMessage, true);

  // Add recipients
  smtpData.addRecipient(inputMessage);

  smtpData.setSendCallback(sendCallback);

  // Start sending Email, can be set callback function to track the status
  if (!MailClient.sendMail(smtpData)) 
  {
    Serial.println("Error sending Email, " + MailClient.smtpErrorReason());
    return false;
  }
  // Clear all data from Email object to free memory
  smtpData.empty();
  return true;
}

// Callback function to get the Email sending status
void sendCallback(SendStatus msg) 
{
  // Print the current status
  Serial.println(msg.info());

  // Do something when complete
  if (msg.success()) 
  {
    Serial.println("----------------");
  }
}

//==================================================================================================
//==================================================================================================
void sendDatatoServer()
{
  //Sending data to server
  board["Temp"] = temp_avg;
  board["Humid"] = hum_avg;
  board["HeatI"] = heatI_avg;
  board["Pm"] = PM_avg;
  board["Meth"] = mq4_avg;
  board["Co"] = mq7_avg;
  board["Fg"] = mq9_avg;
  board["Co2"] = mq135_avg;
  
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());  

  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;
  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) 
  {
    events.send("ping",NULL,millis());
    lastEventTime = millis();
  } 
}

//==================================================================================================

//====================================FUNCTIONS FOR LED BLINKING====================================

void blinkit()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 750) 
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;  
    if(ledState == LOW)
    {
      ledState = HIGH;                        
    }    
    else
    {
      ledState = LOW;
    }
  }
  digitalWrite(led, ledState);
}

void rapid_blinkit()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 50) 
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;  
    if(ledState == LOW)
    {
      ledState = HIGH;                        
    }    
    else
    {
      ledState = LOW;
    }
  }
  digitalWrite(led, ledState);
}
//===================================================================================================

//========================================DISPLAY FUNCTION===========================================

void updatedisplay()
{
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Temp = ");
    lcd.print(temp_avg);
    lcd.print(" °C");
    lcd.setCursor(0,1);
    lcd.print("Humid = ");
    lcd.print(hum_avg);
    lcd.println(" %");
    lcd.setCursor(0,2);
    lcd.print("HeatIndex = ");
    lcd.print(heatI_avg);
    lcd.println(" °C");
    lcd.setCursor(0,3);
    lcd.print("PM = ");
    lcd.print(PM_avg);
    lcd.println(" ug/m3");
    Serial.println("Screen 1");
    delay(500);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Methane = ");
    lcd.print(mq4_avg);
    lcd.print(" ppm");
    lcd.setCursor(0,1);
    lcd.print("CO = ");
    lcd.print(mq7_avg);
    lcd.println(" ppm");
    lcd.setCursor(0,2);
    lcd.print("Flame = ");
    lcd.print(mq9_avg);
    lcd.println(" ppm");
    lcd.setCursor(0,3);
    lcd.print("CO2 = ");
    lcd.print(mq135_avg);
    lcd.println(" ppm");
    Serial.println("Screen 2");
    delay(500);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Location1 : ");
    lcd.print(loc1_status);
    lcd.setCursor(0,1);
    lcd.print("Location2 : ");
    lcd.print(loc2_status);
    lcd.setCursor(0,2);
    lcd.print("Location3 : ");
    lcd.print(loc3_status);
    lcd.setCursor(0,3);
    lcd.print("POSTING data...");
    Serial.println("Screen 3");
    delay(500);

   //==============================================================================================
    
//    lcd.clear();
//    lcd.setCursor(0,0);
//    lcd.print("Temp = ");
//    lcd.print(temp_avg);
//    lcd.print(" °C");
//    lcd.setCursor(0,1);
//    lcd.print("Humid = ");
//    lcd.print(hum_avg);
//    lcd.println(" %");
//    delay(500);
//
//    lcd.clear();
//    lcd.setCursor(0,0);
//    lcd.print("HeatIn = ");
//    lcd.print(heatI_avg);
//    lcd.println("°C");
//    lcd.setCursor(0,1);
//    lcd.print("PM = ");
//    lcd.print(PM_avg);
//    lcd.println(" ug/m3");
//    Serial.println("Screen 1");
//    delay(500);
//
//    lcd.clear();
//    lcd.setCursor(0,0);
//    lcd.print("Methane = ");
//    lcd.print(mq4_avg);
//    lcd.print(" ppm");
//    lcd.setCursor(0,1);
//    lcd.print("CO = ");
//    lcd.print(mq7_avg);
//    lcd.println(" ppm");
//    delay(500);
//
//    lcd.clear();
//    lcd.setCursor(0,0);
//    lcd.print("Flame = ");
//    lcd.print(mq9_avg);
//    lcd.println(" ppm");
//    lcd.setCursor(0,1);
//    lcd.print("CO2 = ");
//    lcd.print(mq135_avg);
//    lcd.println(" ppm");
//    Serial.println("Screen 2");
//    delay(500);
//
//    lcd.clear();
//    lcd.setCursor(0,0);
//    lcd.print("Loc1 : ");
//    lcd.print(loc1_status);
//    lcd.setCursor(0,1);
//    lcd.print("Loc2 : ");
//    lcd.print(loc2_status);
//    delay(500);
//
//    lcd.clear();
//    lcd.setCursor(0,0);
//    lcd.print("Loc3 : ");
//    lcd.print(loc3_status);
//    lcd.setCursor(0,1);
//    lcd.print("POSTING data...");
//    Serial.println("Screen 3");
//    delay(500);
}

//===================================================================================================

//===================================================================================================
//void codeForTask1( void * parameter )
//{
//  for (;;) 
//  {
//    Serial.println("*****************************CORE 0*****************************");
//    updatedisplay();
//    sendDatatoServer();
//  }
//}
//===================================================================================================

void setup()
{
  Serial.begin(9600);
  lcd.init();       
  lcd.backlight();            // Make sure backlight is on
  lcd.clear();

  pinMode(led, OUTPUT);

  // Initialize SPIFFS
  if(!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  
  // Set the device as a Station and Soft Access Point simultaneously
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    Serial.println("Setting as a Wi-Fi Station..");
    delay(1000);
  }
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  //--------------------------------------------------------------------------------------------------
  
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.flush();
  //--------------------------------------------------------------------------------------------------
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/Location1.html", String(), false);
  });
  
  // Route to load .css file
  server.on("/Location1.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/Location1.css", "text/css");
  });
  
  events.onConnect([](AsyncEventSourceClient *client)
  {
    if(client->lastId()){
        Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
      }
      // send event with message "hello!", id current millis and set reconnect delay to 1 second
      client->send("hello!", NULL, millis(), 1000);
  });
  server.addHandler(&events);  
  server.begin();

  //--------------------------------------------------------------------------------------------------
//  xTaskCreatePinnedToCore(
//    codeForTask1,            /* Task function. */
//    "Task_1",                 /* name of task. */
//    5000,                    /* Stack size of task */
//    NULL,                     /* parameter of the task */
//    1,                        /* priority of the task */
//    &Task1,                   /* Task handle to keep track of created task */
//    0);                       /* Core */
  //--------------------------------------------------------------------------------------------------
}

void loop()
{
  Serial.println("****************************CORE 1*****************************");
  //============================STORING RECEIVED VALUES IN VARIABLES=================================
  
  temp_val_1 = boardsStruct[0].dht_temp;
  hum_val_1 = boardsStruct[0].dht_humid;
  heatI_1 = boardsStruct[0].heat_index;
  mq4_1 = boardsStruct[0].mq4_val;
  mq7_1 = boardsStruct[0].mq7_val;
  mq9_1 = boardsStruct[0].mq9_val;
  mq135_1 = boardsStruct[0].mq135_val;
  PM_1 = boardsStruct[0].PM_val;

  temp_val_2 = boardsStruct[1].dht_temp;
  hum_val_2 = boardsStruct[1].dht_humid;
  heatI_2 = boardsStruct[1].heat_index;
  mq4_2 = boardsStruct[1].mq4_val;
  mq7_2 = boardsStruct[1].mq7_val;
  mq9_2 = boardsStruct[1].mq9_val;
  mq135_2 = boardsStruct[1].mq135_val;
  PM_2 = boardsStruct[1].PM_val;

  temp_val_3 = boardsStruct[2].dht_temp;
  hum_val_3 = boardsStruct[2].dht_humid;
  heatI_3 = boardsStruct[2].heat_index;
  mq4_3 = boardsStruct[2].mq4_val;
  mq7_3 = boardsStruct[2].mq7_val;
  mq9_3 = boardsStruct[2].mq9_val;
  mq135_3 = boardsStruct[2].mq135_val;
  PM_3 = boardsStruct[2].PM_val;

  //===================================================================================================

  temp_avg = (temp_val_1 + temp_val_2 + temp_val_3)/3;
  hum_avg = (hum_val_1 + hum_val_2 + hum_val_3)/3;
  heatI_avg = (heatI_1 + heatI_2 + heatI_3)/3;
  mq4_avg = (mq4_1 + mq4_2 + mq4_3)/3;
  mq7_avg = (mq7_1 + mq7_2 + mq7_3)/3;
  mq9_avg = (mq9_1 + mq9_2 + mq9_3)/3;
  mq135_avg = (mq135_1 + mq135_2 + mq135_3)/3;
  PM_avg = (PM_1 + PM_2 + PM_3)/3;

//  temp_avg = 29.05;
//  hum_avg = 50.00;
//  heatI_avg = 26.03;
//  mq4_avg = 20.0;
//  mq7_avg = 550.0;
//  mq9_avg = 30.0;
//  mq135_avg = 300.0;
//  PM_avg = 90.0;

//  updatedisplay();
  sendDatatoServer();

  //===================================================================================================

  if ((mq4_1 >= 2.0) || (mq7_1 >= 5.0) || (mq9_1 >= 25.0) || (mq135_1 >= 50.0))
  {
      rapid_blinkit();
      String emailMessage = "Location 1 UNSAFE!!!!";
      loc1_status = "UNSAFE!!";
      if(sendEmailNotification(emailMessage)) 
      {
        Serial.println(emailMessage);
      }
      else 
      {
        Serial.println("Email failed to send");
      }
  }
  else if ((mq4_2 >= 2.0) || (mq7_2 >= 5.0) || (mq9_2 >= 25.0) || (mq135_2 >= 50.0))
  {
      rapid_blinkit();
      loc2_status = "UNSAFE!!";
      String emailMessage = "Location 2 UNSAFE!!!!";
      if(sendEmailNotification(emailMessage)) 
      {
        Serial.println(emailMessage);
      }
      else 
      {
        Serial.println("Email failed to send");
      }
  }
  else if ((mq4_3 >= 2.0) || (mq7_3 >= 5.0) || (mq9_3 >= 25.0) || (mq135_3 >= 50.0))
  {
      rapid_blinkit();
      loc3_status = "UNSAFE!!";
      String emailMessage = "Location 3 UNSAFE!!!!";
      if(sendEmailNotification(emailMessage)) 
      {
        Serial.println(emailMessage);
      }
      else 
      {
        Serial.println("Email failed to send");
      }
  }
  else
  {
    loc1_status = "SAFE";
    loc2_status = "SAFE";
    loc3_status = "SAFE";
  }

  //===================================================================================================
  if (WiFi.status() == WL_CONNECTED) 
  {
    static bool flag = false;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) 
    {
      Serial.println("Failed to obtain time");
      return;
    }

    String temp_val_1 = String(boardsStruct[0].dht_temp);
    String hum_val_1 = String(boardsStruct[0].dht_humid);
    String heatI_1 = String(boardsStruct[0].heat_index);
    String mq4_1 = String(boardsStruct[0].mq4_val);
    String mq7_1 = String(boardsStruct[0].mq7_val);
    String mq9_1 = String(boardsStruct[0].mq9_val);
    String mq135_1 = String(boardsStruct[0].mq135_val);
    String PM_1 = String(boardsStruct[0].PM_val);
  
    String temp_val_2 = String(boardsStruct[1].dht_temp);
    String hum_val_2 = String(boardsStruct[1].dht_humid);
    String heatI_2 = String(boardsStruct[1].heat_index);
    String mq4_2 = String(boardsStruct[1].mq4_val);
    String mq7_2 = String(boardsStruct[1].mq7_val);
    String mq9_2 = String(boardsStruct[1].mq9_val);
    String mq135_2 = String(boardsStruct[1].mq135_val);
    String PM_2 = String(boardsStruct[1].PM_val);
  
    String temp_val_3 = String(boardsStruct[2].dht_temp);
    String hum_val_3 = String(boardsStruct[2].dht_humid);
    String heatI_3 = String(boardsStruct[2].heat_index);
    String mq4_3 = String(boardsStruct[2].mq4_val);
    String mq7_3 = String(boardsStruct[2].mq7_val);
    String mq9_3 = String(boardsStruct[2].mq9_val);
    String mq135_3 = String(boardsStruct[2].mq135_val);
    String PM_3 = String(boardsStruct[2].PM_val);

    
    String urlFinal = "https://script.google.com/macros/s/"+GOOGLE_SCRIPT_ID+"/exec?temp_1=" + temp_val_1 + "&humid_1=" + hum_val_1 + "&heati_1=" + heatI_1 + 
    "&mq4_1=" + mq4_1 + "&mq7_1=" + mq7_1 + "&mq9_1=" + mq9_1 + "&mq135_1=" + mq135_1 + "&pm_1=" + PM_1 + "&temp_2=" + temp_val_2 + "&humid_2=" + hum_val_2 + "&heati_2=" + heatI_2 + 
    "&mq4_2=" + mq4_2 + "&mq7_2=" + mq7_2 + "&mq9_2=" + mq9_2 + "&mq135_2=" + mq135_2 + "&pm_2=" + PM_2 + "&temp_3=" + temp_val_3 + "&humid_3=" + hum_val_3 + "&heati_3=" + heatI_3 + 
    "&mq4_3=" + mq4_3 + "&mq7_3=" + mq7_3 + "&mq9_3=" + mq9_3 + "&mq135_3=" + mq135_3 + "&pm_3=" + PM_3;
    Serial.println("POST data to spreadsheet:");
    
    //Serial.println(urlFinal);
    HTTPClient http;
    http.begin(urlFinal.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET(); 
    Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);
    //---------------------------------------------------------------------
    //getting response from google sheet
    String payload;
    if (httpCode > 0) 
    {
        payload = http.getString();
//        Serial.println("Payload: "+payload);    
    }
    //---------------------------------------------------------------------
    http.end();
    unsigned long currentMillis = millis();
    Serial.println(currentMillis);
  }
  updatedisplay();
  delay(500);
}
