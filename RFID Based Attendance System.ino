/* ============================================================
 * Project: RFID Based Attendance System for automated attendance tracking and real-time data logging
 * ============================================================ */


#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecureBearSSL.h>
#include<LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define RST_PIN  D3     // Configurable, see typical pin layout above
#define SS_PIN   D4     // Configurable, see typical pin layout above
#define BUZZER   D2     // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Instance of the class
MFRC522::MIFARE_Key key;  
ESP8266WiFiMulti WiFiMulti;
MFRC522::StatusCode status;      


const char* ssid = "Tenda22"; //--> Your wifi name or SSID.
const char* password = "@tendaa22@"; //--> Your wifi password.

//----------------------------------------Host & httpsPort
const char* host = "script.google.com";
const int httpsPort = 443;
//----------------------------------------

WiFiClientSecure client; //--> Create a WiFiClientSecure object.

String GAS_ID = "AKfycbwXBrgmaD42E6yd7GFMlZGiuBTGM5UQ9Y_yPiP_kH4TR5aofLoZZpdlvefg9hjztlrHdA"; //--> spreadsheet script ID


/* Be aware of Sector Trailer Blocks */
int blockNum = 2;  

/* Create another array to read data from Block */
/* Legthn of buffer should be 2 Bytes more than the size of Block (16 Bytes) */
byte bufferLen = 18;
byte readBlockData[18];
String card_holder_name;
String card_holder_id;

int blocks[] = {4,5};
#define blocks_len  (sizeof(blocks) / sizeof(blocks[0]))

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(500);

 pinMode(D0, OUTPUT);

  lcd.init();   // initialize the lcd 
  // Print a message to the LCD.
  lcd.backlight();
  
  Wire.begin (D2, D1);
  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("Connecting");
  lcd.setCursor(0,1);
  lcd.print("Please Wait... ");
  delay(500);
  
  WiFi.begin(ssid, password); //--> Connect to your WiFi router
  Serial.println("");
    
  //----------------------------------------Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
 
  Serial.println("");
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  lcd.clear();
  delay(100);
  lcd.setCursor(0,0);
  lcd.print("Scan Your Card");

  client.setInsecure();

  /* Initialize SPI bus */
  SPI.begin();
}

void loop() {

   /* Initialize MFRC522 Module */
  mfrc522.PCD_Init();
  /* Look for new cards */
  /* Reset the loop if no new card is present on RC522 Reader */
  if ( ! mfrc522.PICC_IsNewCardPresent()) {return;}
  /* Select one of the cards */
  if ( ! mfrc522.PICC_ReadCardSerial()) {return;}
  /* Read data from the same block */
  Serial.println();
  Serial.println(F("Reading last data from RFID..."));  
  //------------------------------------------------------------------------

  for (byte i = 0; i < blocks_len; i++) {
    ReadDataFromBlock(blocks[i], readBlockData);
    if(i == 0){
      card_holder_name = String((char*)readBlockData);
      card_holder_name.trim();
    }
    else{
      card_holder_id = String((char*)readBlockData);
      card_holder_id.trim();
    }
  }

  Serial.println(card_holder_name);
  Serial.println(card_holder_id);


  lcd.clear();
  delay(100);
  lcd.setCursor(0,0);
  lcd.print(card_holder_name);
  lcd.setCursor(0,1);
  lcd.print(card_holder_id);
  
  digitalWrite(D0, HIGH);
  delay(500);
  
  delay(1000);
  
  lcd.clear();
  delay(100);
  lcd.setCursor(0,0);
  lcd.print("Card Reading");
  lcd.setCursor(0,1);
  lcd.print("Successfull");

  tone(D0, 250); 
  digitalWrite(D0, LOW);
  delay(50);
  
  sendData(card_holder_name, card_holder_id); //--> Calls the sendData Subroutine
}


//// Part 2


// Subroutine for sending data to Google Sheets
void sendData(String st_name, String st_id) {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  
  //----------------------------------------Connect to Google host
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    Serial.print("Try Again");
    
    lcd.clear();
    delay(100);
    lcd.setCursor(0,0);
    lcd.print("Sending Fail");
     lcd.setCursor(0,1);
    lcd.print("Plz Try Again");

   delay(2000);
  lcd.clear();
  delay(100);
  lcd.setCursor(0,0);
  lcd.print("Scan Your Card");

  
    return;
  }

  //----------------------------------------Processing data and sending data
  String string_name =  String(st_name);
  // String string_temperature =  String(tem, DEC); 
  String string_id =  String(st_id); 
  String url = "/macros/s/" + GAS_ID + "/exec?st_names=" + string_name + "&st_ids=" + string_id;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("request sent");
  lcd.clear();
  delay(100);
  lcd.setCursor(0,0);
  lcd.print("Request sent");
  lcd.setCursor(0,1);
  lcd.print("Please Wait... ");


  // Checking whether the data was sent successfully or not
  
    while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("Attendence received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("Attendence successfull!");
  } else {
    Serial.println("Try Again");
  }
  Serial.print("reply was : ");
  Serial.println(line);

  lcd.clear();
  delay(100);
  lcd.setCursor(0,0);
  lcd.print("Attendence");
  lcd.setCursor(0,1);
  lcd.print("Successfull!");

  delay(2000);
  lcd.clear();
  delay(100);
  lcd.setCursor(0,0);
  lcd.print("Scan Your Card");
  
  Serial.println("Thank You");
  Serial.println("==========");
  Serial.println();
  
  } 


//  ReadDataFromBlock() function

void ReadDataFromBlock(int blockNum, byte readBlockData[]) 
{ 
  //----------------------------------------------------------------------------
  /* Prepare the ksy for authentication */
  /* All keys are set to FFFFFFFFFFFFh at chip delivery from the factory */
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  //----------------------------------------------------------------------------
  /* Authenticating the desired data block for Read access using Key A */
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));
  //----------------------------------------------------------------------------s
  if (status != MFRC522::STATUS_OK){
     Serial.print("Authentication failed for Read: ");
     Serial.println(mfrc522.GetStatusCodeName(status));
     return;
  }
  //----------------------------------------------------------------------------
  else {
    Serial.println("Authentication success");
  }
 
  // Reading data from the Block 
  
  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Reading failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }

  else {
    readBlockData[16] = ' ';
    readBlockData[17] = ' ';
    Serial.println("Block was read successfully");  
  }
  
}


//// Part 3



function doGet(e) { 
Logger.log( JSON.stringify(e) );
var result = 'Ok';
if (e.parameter == 'undefined') {
result = 'No Parameters';
}
else {
var sheet_id = '1vw-1BRseNvSSCDx73HLsuVAIq_Kx46nqp0qn_DnrIXw'; // Spreadsheet ID
var sheet = SpreadsheetApp.openById(sheet_id).getActiveSheet();
var newRow = sheet.getLastRow() + 1; 
var rowData = [];
var hours = +6
var Curr_Date =new Date(new Date().setHours(new Date().getHours() + hours));
rowData[0] = Curr_Date; // Date in column A

var Curr_Time = Utilities.formatDate(Curr_Date, "Asia/dhaka", 'HH:mm:ss');
rowData[1] = Curr_Time; // Time in column B

for (var param in e.parameter) {
Logger.log('In for loop, param=' + param);
var value = stripQuotes(e.parameter[param]);
Logger.log(param + ':' + e.parameter[param]);

switch (param) {
case 'st_names':
rowData[2] = value; // Temperature in column C
result = 'Temperature Written on column C'; 
break;

case 'st_ids':
rowData[3] = value; // Humidity in column D
result += ' ,Humidity Written on column D'; 
break; 

default:
result = "unsupported parameter";
}
}
Logger.log(JSON.stringify(rowData));
var newRange = sheet.getRange(newRow, 1, 1, rowData.length);
newRange.setValues([rowData]);
}
return ContentService.createTextOutput(result);
}
function stripQuotes( value ) {
return value.replace(/^["']|['"]$/g, "");
}