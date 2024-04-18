//=====================================DECLARING LIBRARIES========================================
//================================================================================================
#include <Wire.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <esp_now.h>    //library for espnow 
#include <esp_wifi.h>
#include <WiFi.h>

//TaskHandle_t Task1;
//==============================DECLARING FUNCTIONS FOR LCD DISPLAY===============================
//================================================================================================
LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x3F for a 16 chars and 2 line display //0x27 in node3

//=====================================DEFINING MQ SENSOR PINS====================================
//================================================================================================
#define MQ4_pin     33       //used for measuring CH4 (methane) and natural gas
#define MQ7_pin     35       //used for measuring CO (carbon monoxide) and hydrogen (in traces)
#define MQ9_pin     34       //used for detecting LPG (gas for house) (flammable gases better detection)
#define MQ135_pin   32       //used for detecting CO2 (carbon dioxide) and Ethanol (alcohol detection)

//required for detecting the safe limit (digital pin initialise)
//----------------------------------------------------------------
#define mq4_dig     26
#define mq7_dig     13
#define mq9_dig     14
#define mq135_dig   17

//==============================DEFINING SENSOR HEAT RESISTANCE FOR MQ SENSORS====================
//the value of resistance is in Kilo Ohms=========================================================
//================================================================================================

#define MQ4_RL      20.0      //20 kilo-ohms
#define MQ7_RL      10.0      //10 kilo-ohms
#define MQ9_RL      10.0      //10 kilo-ohms
#define MQ135_RL    20.0      //20 kilo-ohms

//==============================DEFINING VARIABLES FOR MQ SENSORS=================================
//================================================================================================
//callibrated resistance (R0) will be placed automatically after callibration is done
//-----------------------------------------------------------------------------------
float mq4_res0    = 0.848;       
float mq7_res0    = 0.735;       
float mq9_res0    = 0.935;
float mq135_res0  = 0.942;

//sensor heater resistance (Rs) will be calculated from analogread
//----------------------------------------------------------------
int mq4_RS        = 0;             
int mq7_RS        = 0;
int mq9_RS        = 0;
int mq135_RS      = 0;

//required for calculating the ratio (mq4_RS / mq4_res0)
//----------------------------------------------------------------
float mq4_ratio     = 0.0;
float mq7_ratio     = 0.0;
float mq9_ratio     = 0.0;
float mq135_ratio   = 0.0;

//required for calculating the final PPM values
//----------------------------------------------------------------
float mq4_ppm     = 0.0;
float mq7_ppm     = 0.0;
float mq9_ppm     = 0.0;
float mq135_ppm   = 0.0;

//==============================DEFINING PM2.5 DUST SENSOR========================================
//================================================================================================
#define PM_pin        39        //used for detecting dust of dimension 2.5u (microns)
#define LED_POWER_PM  16

unsigned int samplingTime = 280;
unsigned int deltaTime = 40;
unsigned int sleepTime = 9680;

float voMeasured = 0.0;
float calcVoltage = 0.0;
float dustDensity = 0.0;

//===================================DEFINING DHT11 functions ====================================
//================================================================================================
#define DHT11_pin     15        //used for detecting temperature and humidity
DHT dht(DHT11_pin, DHT11);

//=================================DEFINING variables for DHT11===================================
//================================================================================================
float temp = 0.0;
float humid = 0.0;
float heatIndex = 0.0;

//================================================================================================
//================================================================================================
int led = 2;
int ledState = LOW;   // ledState used to set the LED

//================================================================================================
//=======================CREDENTIALS AND FUNCTIONS FOR ESP NOW====================================
// Set your Board ID (ESP32 Sender #1 = BOARD_ID 1, ESP32 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID 3

//-------------------------------------------------------------------------------------
uint8_t RxMACaddress[] = {0x70, 0xB8, 0xF6, 0x5B, 0xFE, 0x48}; //C0:49:EF:E7:C5:70  40:22:D8:7B:64:D8  70:B8:F6:5B:FE:48

//-------------------------------------------------------------------------------------

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message 
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
} struct_message;

//Create a struct_message called myData
struct_message myData;

unsigned long previousMillis = 0;   // Stores last time temperature was published

//unsigned int readingId = 0;
//================================================================================================
// Insert your SSID
//constexpr char WIFI_SSID[] = "TP-Link_907E";
constexpr char WIFI_SSID[] = "Ritam";

int32_t getWiFiChannel(const char *ssid) 
{
  if (int32_t n = WiFi.scanNetworks()) 
  {
      for (uint8_t i=0; i<n; i++) 
      {
          if (!strcmp(ssid, WiFi.SSID(i).c_str())) 
          {
              return WiFi.channel(i);
          }
      }
  }
  return 0;
}
//================================================================================================

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

//================================================================================================

//===================================FUNCTIONS FOR LED BLINKING IN MODES==========================
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

void rapid_blink()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 350) 
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
//================================================================================================
void updatedisplay()
{
  //===================================LCD PRINTING STATEMETNS===================================
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("MQ4 = ");
   lcd.print(mq4_ppm);
   lcd.println(" ppm");
   lcd.setCursor(0,1);
   lcd.print("MQ7 = ");
   lcd.print(mq7_ppm);
   lcd.println(" ppm");

   delay(500);

   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("MQ9 = ");
   lcd.print(mq9_ppm);
   lcd.println(" ppm");
   lcd.setCursor(0,1);
   lcd.print("MQ135 = ");
   lcd.print(mq135_ppm);
   lcd.println(" ppm");

   delay(500);

   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("T=");
   lcd.print(temp);
   lcd.print("C");
   lcd.setCursor(0,1);
   lcd.print("RT=");
   lcd.print(heatIndex);
   lcd.print("C");
   lcd.setCursor(8,0);
   lcd.print("|");
   lcd.setCursor(8,1);
   lcd.print("|");
   lcd.setCursor(9,0);
   lcd.print("H=");
   lcd.print(humid);
   lcd.print("%");
   lcd.setCursor(9,1);
   lcd.print("PM=");
   lcd.print(dustDensity);
   //lcd.print("ppm");
   delay(500);

   //=============================================================================================
}

//===================================================================================================
//void codeForTask1( void * parameter )
//{
//  for (;;) 
//  {
//    Serial.println("*****************************CORE 0*****************************");
//    updateDisplay();
//  }
//}
//===================================================================================================

void setup() 
{
  Serial.begin(115200);       //setting up the baud rate
  dht.begin();                //initializing the DHT11 sensor
  pinMode(LED_POWER_PM,OUTPUT);

  lcd.init();       
  lcd.backlight();            // Make sure backlight is on

  //===============================================================================================
  //===============================SETTING UP FOR ESP NOW==========================================

  // Set device as a Wi-Fi Station and set channel
  WiFi.mode(WIFI_STA);

  int32_t channel = getWiFiChannel(WIFI_SSID);

  WiFi.printDiag(Serial); // Uncomment to verify channel number before
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  WiFi.printDiag(Serial); // Uncomment to verify channel change after

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  //Register peer
  
//  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, RxMACaddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  //Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
  
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
   //========================DECLARING FUNCTIONS AND INPUTS FOR MQ SENSORS========================
   //=============================================================================================
   mq4_RS     = analogRead(MQ4_pin);
   mq7_RS     = analogRead(MQ7_pin);
   mq9_RS     = analogRead(MQ9_pin);
   mq135_RS   = analogRead(MQ135_pin);

   int read_mq4   = digitalRead(mq4_dig);
   int read_mq7   = digitalRead(mq7_dig);
   int read_mq9   = digitalRead(mq9_dig);
   int read_mq135 = digitalRead(mq135_dig);

   mq4_ratio    = mq4_RS / mq4_res0;
   mq7_ratio    = mq7_RS / mq7_res0;
   mq9_ratio    = mq9_RS / mq9_res0;
   mq135_ratio  = mq135_RS / mq135_res0;

   mq4_ppm   = 1000.0 * pow(10.0, ((log10(mq4_ratio) - 0.3593) / (-0.4323)));
   mq7_ppm   = 1000.0 * pow(10.0, ((log10(mq7_ratio) - 0.3315) / (-0.4443)));
   mq9_ppm   = 1000.0 * pow(10.0, ((log10(mq9_ratio) - 0.3516) / (-0.3943)));
   mq135_ppm = 1000.0 * pow(10.0, ((log10(mq135_ratio) - 0.2796) / (-0.3389)));

   if ((read_mq4 == HIGH) || (read_mq7 == HIGH) || (read_mq9 == HIGH) || (read_mq135 == HIGH))
   {
      rapid_blink();
   }

   //=============================================================================================
   //========================DECLARING FUNCTIONS AND INPUTS FOR DHT SENSORS=======================
   //=============================================================================================
   // Reading temperature or humidity takes about 250 milliseconds!
   // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
   humid = dht.readHumidity();
   // Read temperature as Celsius (the default)
   temp = dht.readTemperature();
  
    // Check if any reads failed and exit early (to try again).
//   if (isnan(humid) || isnan(temp)) 
//   {
//      Serial.println(F("Failed to read from DHT sensor!"));
//      return;
//   }

   // Compute heat index in Celsius (isFahreheit = false)
   heatIndex = dht.computeHeatIndex(temp, humid, false);
   
   //=============================================================================================
   //=========================DECLARING LOOP FUNCTIONS FOR PM2.5==================================
   digitalWrite(LED_POWER_PM,LOW);
   delayMicroseconds(samplingTime);

   voMeasured = analogRead(PM_pin);

   delayMicroseconds(deltaTime);
   digitalWrite(LED_POWER_PM,HIGH);
   delayMicroseconds(sleepTime);

   calcVoltage = voMeasured * (5.0/4096);
   dustDensity = 0.17 * calcVoltage - 0.1;

   if ( dustDensity < 0)
   {
     dustDensity = 0.00;
   }
   //=============================================================================================
   
   //===================================ESP NOW SENDER FUNCTIONS==================================
   myData.ID = BOARD_ID;
   myData.dht_temp = temp;
   myData.dht_humid = humid;
   myData.heat_index = heatIndex;
   myData.mq4_val = mq4_ppm;
   myData.mq7_val = mq7_ppm;
   myData.mq9_val = mq9_ppm;
   myData.mq135_val = mq135_ppm;
   myData.PM_val = dustDensity;

   //-------------------------------------------------------------------------------------
   esp_err_t result = esp_now_send(RxMACaddress, (uint8_t *) &myData, sizeof(myData));
   //-------------------------------------------------------------------------------------
   if (result == ESP_OK) 
   {
      Serial.println("Sent with success");
      blinkit();
   }
   else
   {
      Serial.println("Error sending the data");
   }
   //-------------------------------------------------------------------------------------
   updatedisplay();
   delay(500); //main delay of the code
   
}
