#include <WiFi.h>
#include <HTTPClient.h>
#include <vector>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

/********************************************************************************/
// Things to change
const char * ssid = "Galaxy A34 5G 4F72";
const char * password = "rfid1234";
String GOOGLE_SCRIPT_ID = "AKfycbwAoHiaoAtAOa4pD-L1fL1t5taHA5WkN9CxdahIOoEy91ZL9YzO3dWw4L8DkbRjLdL1";

const int sendInterval = 500;
/********************************************************************************/
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);  

WiFiClientSecure client;

#include "SPI.h"      // SPI library
#include "MFRC522.h"  // RFID library (https://github.com/miguelbalboa/rfid)

const int buzzer = 2;
const int pinRST = 0;
const int pinSDA = 5;

MFRC522 mfrc522(pinSDA, pinRST);  // Set up mfrc522 on the Arduino

#include <RTClib.h>
#include <Wire.h>
RTC_DS3231 rtc;
char d[32];
char t[32];
String Attendance;

// Define check in time
const int checkInHour = 14;
const int checkInMinute = 20;
//Variable to hold user check in
int userCheckInHour;
int userCheckInMinute;

//String Date, Time, Attendance;

// Define the I2C pins for each device


void sendData(String params) {
   HTTPClient http;
   String url="https://script.google.com/macros/s/AKfycbwAoHiaoAtAOa4pD-L1fL1t5taHA5WkN9CxdahIOoEy91ZL9YzO3dWw4L8DkbRjLdL1/exec?action=write&"+params;
   //Serial.print(url);
   //Serial.print("\nMaking a request");
   http.begin(url);//, root_ca); //Specify the URL and certificate
   int httpCode = http.GET();  
   http.end();
   //Serial.println(": done "+httpCode);
}

void removeBracketsAndSpaces(String &input) {
  input.replace("[", "");
  input.replace("]", "");
  input.trim();
}

void spreadsheet_comm(std::vector<String> &numbers, std::vector<String> &names) {
  HTTPClient http;
  String url = "https://script.google.com/macros/s/AKfycbwAoHiaoAtAOa4pD-L1fL1t5taHA5WkN9CxdahIOoEy91ZL9YzO3dWw4L8DkbRjLdL1/exec?action=read&Column=A";
  Serial.println("\n\nScanning...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scanning..."); 

  http.begin(url.c_str()); // Specify the URL and certificate
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  String payload = http.getString();
  //Serial.println(payload);

  //Serial.println("function has been reached");

  int tokenCount = 0;
  while (payload.length() > 0) {
    //Serial.println("while loop has been cleared");
    int openBracketIndex = payload.indexOf('[');
    int closeBracketIndex = payload.indexOf(']');
    if (openBracketIndex != -1 && closeBracketIndex != -1) {
      //Serial.println("first if loop has been cleared");
      String token = payload.substring(openBracketIndex + 1, closeBracketIndex);
      int commaIndex = token.indexOf(',');
      if (commaIndex != -1) {
        // Remove square brackets and spaces from numbers
        String numberStr = token.substring(0, commaIndex);
        removeBracketsAndSpaces(numberStr);
        
        //Serial.print("Number: ");
        //Serial.println(numberStr);
        numbers.push_back(numberStr);
        
        //Serial.print("Name: ");
        //Serial.println(token.substring(commaIndex + 1));
        names.push_back(token.substring(commaIndex + 1));
      }
      payload = payload.substring(closeBracketIndex + 1);
    } else {
      break;
    }
  }
}

String removeDoubleQuotes(String input) {
  // Check if the input string starts with a double quote
  if (input.startsWith("\"")) {
    input = input.substring(1);
  }

  // Check if the input string ends with a double quote
  if (input.endsWith("\"")) {
    input = input.substring(0, input.length() - 1);
  }

  return input;
}

void setup() {
  Wire.begin();
  SPI.begin();         // open SPI connection
  //Wire.setPins(rfidSDA, rfidSCL);
  mfrc522.PCD_Init();  // Initialize Proximity Coupling Device (PCD)
  Serial.begin(9600);  // open serial connection

  //Wire.begin();

  //Wire.setPins(rtcSDA, rtcSCL);
  rtc.begin();
  //rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  //Wire.setPins(25, 33); // Switch to the second I2C bus (SDA on pin 25, SCL on pin 33)
  //rtc.adjust(DateTime(2023, 10, 24, 4, 52, 0));

  pinMode(buzzer,OUTPUT);
  delay(10);

  //Wire.setPins(lcdSDA, lcdSCL);
  // initialize the lcd
	lcd.init();
	// Turn on the Backlight
	lcd.backlight();

  

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println("\nStarted");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Started"); 
  Serial.print("Connecting");
  lcd.setCursor(0, 1);
  lcd.print("Connecting..."); 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Ready to go");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Ready to go"); 
}

void loop() {
  // Clear the display buffer
	//lcd.clear(); 
  //Wire.setPins(rtcSDA, rtcSCL);
  DateTime now = rtc.now();
 // Wire.setPins(rfidSDA, rfidSCL);
  if (mfrc522.PICC_IsNewCardPresent()) {
    if (mfrc522.PICC_ReadCardSerial()) {
      digitalWrite(buzzer, HIGH);
      delay(250);
      digitalWrite(buzzer, LOW);
      std::vector<String> numbers;
      std::vector<String> names;
      spreadsheet_comm(numbers, names);
      String UID = "";
     
      for (byte i = 0; i < mfrc522.uid.size; ++i) {
        UID += String(mfrc522.uid.uidByte[i], HEX);
      }

      bool found = false;
      int sheet_length = numbers.size();
      //Serial.println(sheet_length);
      for (int i = 0; i < sheet_length; i++) {
        /*
        Serial.print("Comparing UID: ");
        Serial.print(UID);
        Serial.print(" with numbers[");
        Serial.print(i);
        Serial.print("]: ");
        */
        String stored_UID = removeDoubleQuotes(numbers[i]);
        //Serial.println(stored_UID);

        if (stored_UID == UID) {
          //Serial.print(UID);
          Serial.print("\nWelcome ");

         // Wire.setPins(lcdSDA, lcdSCL);
          lcd.setCursor(0, 0);
	        // print "Hello" at (0, 0)	 
        	lcd.print("Welcome"); 
          
          Serial.println(removeDoubleQuotes(names[i]));
          lcd.setCursor(0, 1);
	        // print "Hello" at (0, 0)	 
	        lcd.print(removeDoubleQuotes(names[i])); 
          found = true;
         // Wire.setPins(rtcSDA, rtcSCL);
          userCheckInHour = now.hour();
          userCheckInMinute = now.minute();
          sprintf(d, "%02d/%02d/%02d", now.day(), now.month(), now.year());
          sprintf(t, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
          String Date = String(d);
          String Time = String(t);
          Serial.print("Student Entry Time: 14:20");
          Serial.print("\nYour Entry Time:");
          Serial.print(Time);
          if ((userCheckInHour < checkInHour) || ((userCheckInHour == checkInHour) && (userCheckInMinute <= checkInMinute))) {
            Attendance = "PRESENT";
            Serial.print("\nYou are on time!");
            //Wire.setPins(lcdSDA, lcdSCL);
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(removeDoubleQuotes(names[i]));
            lcd.setCursor(0,1);
            lcd.print("You are on time!");
          } else {
            Attendance = "LATE";
            Serial.print("\nYou are late...");
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(removeDoubleQuotes(names[i]));
            lcd.setCursor(0,1);
            lcd.print("You are late...");
          }
          sendData("Row=" + String(i+2) + "&Column=E&Value=" + Attendance);
          //delay(300);
          sendData("Row=" + String(i+2) + "&Column=D&Value=" + Time);
          //delay(300);
          sendData("Row=" + String(i+2) + "&Column=C&Value=" + Date);
          
          //delay(sendInterval);
          break;
        }
        
      }
      if (!found){
      //  Wire.setPins(lcdSDA, lcdSCL);
        Serial.print(UID);
        Serial.print(" has not been registered");
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print(UID);
        lcd.setCursor(0,1);
        lcd.print("has not been registered");
      }
    }
  }
}  // Closing bracket for loop() function

//?action=read&Column=A
//?action=write&Row=2&Column=C&Value=HII
