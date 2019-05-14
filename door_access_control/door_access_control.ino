 /*
 * Typical pin layout used:
 * ------------------------------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino     Arduino 
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro   Pro Mini
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin         Pin
 * ------------------------------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST         9
 * SPI SS      SDA(SS)      10            53        D10        10               10          A4
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16          11  
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14          12
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15          13
 */ 
//  RFID
#include <SPI.h>
#include <MFRC522.h>
//  RFID pin
#define SS_PIN A4
#define RST_PIN 9
// Store the legal rfid car UID.
String legal_rfid_card_uid[] = {"33 55 88 77", "159 166 21 200"};

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];


// RGB LED
int val = 0;        // val is used to store the 0~255 value, and control the color of led
#define redpin  3   //select the pwm pin for the red LED
#define bluepin 5   // select the pwm pin for the blue LED
#define greenpin 6  // select the pwm pin for the green LED


//  Bz and Relay
#include "pitches.h"
#define bz 10      //  define bz pin.
#define relay 7    //  define relay pin.
int correct_melody = NOTE_C5;

int duration = 100;  // 100 miliseconds 

void setup() {
  //  RFID initialization
  Serial.begin(9600);
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  //  RGB LED initialization
  pinMode(redpin, OUTPUT);
  pinMode(bluepin, OUTPUT);
  pinMode(greenpin, OUTPUT);
  for(val=255; val>0; val--){
    analogWrite(redpin, val);
    analogWrite(bluepin, 255-val);
    analogWrite(greenpin, 128-val);
    delay(1);
  }
  for(val=0; val<255; val++){
    analogWrite(redpin, val);
    analogWrite(bluepin, 255-val);
    analogWrite(greenpin, 128-val);
    delay(1);
  }
  
  //  Bz and relay initialization
  tone(bz, correct_melody, duration);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, HIGH);
  delay(500);
  digitalWrite(relay, LOW);
}

void loop() {
  String match_rfid_card = "";
  analogWrite(redpin, 255);
  analogWrite(bluepin, 0);
  analogWrite(greenpin, 0);
  
  //  RFID
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
   
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);
    Serial.println();
  }
  else Serial.println(F("Card read ."));

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();

  // Store the rfid card UID of detected 
  for(int i=0; i<4; i++){
    if(i == 3){
      match_rfid_card += String(nuidPICC[i]);
    }
    else{
      match_rfid_card += String(nuidPICC[i])+" ";
    }
  }
  
//  Serial.println(match_rfid_card);
  // Determine if it is a legal card
  for(int i=0; i<sizeof(legal_rfid_card_uid); i++){
    // YES
    if(match_rfid_card == legal_rfid_card_uid[i]){
    //  LED
    analogWrite(redpin, 0);
    analogWrite(bluepin, 0);
    analogWrite(greenpin, 255);
    delay(1000);
    //  Bz and Relay
    tone(bz, correct_melody, duration);
    digitalWrite(relay, HIGH);
    delay(3000);
    tone(bz, correct_melody, duration);
    digitalWrite(relay, LOW);
  }
  // NO
  else{
    analogWrite(redpin, 0);
    delay(100);
    analogWrite(redpin, 255);
    analogWrite(bluepin, 0);
    analogWrite(greenpin, 0);
    digitalWrite(relay, LOW);
    tone(bz, correct_melody, duration);
    delay(200);
    tone(bz, correct_melody, duration);
  }
 } 
}

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
