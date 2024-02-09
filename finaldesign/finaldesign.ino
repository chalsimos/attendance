#include <SD.h>
#include <DS1302.h>
#include <SoftwareSerial.h>
#include <Adafruit_MLX90614.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>
#include <NfcAdapter.h>

#if 0
#include <SPI.h>
#include <PN532_SPI.h>
#include "PN532.h"
PN532_SPI pn532spi(SPI, 10);
NfcAdapter nfc = NfcAdapter(pn532spi);
// PN532 nfc(pn532spi);

#elif 1
#include <PN532_HSU.h>
#include <PN532.h>
PN532_HSU pn532hsu(Serial1);
NfcAdapter nfc = NfcAdapter(pn532hsu);
// PN532 nfc(pn532hsu);

#else
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <NfcAdapter.h>

PN532_I2C pn532i2c(Wire);
NfcAdapter nfc = NfcAdapter(pn532i2c);
// PN532 nfc(pn532i2c);
#endif

DS1302 rtc(8,9,10);
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x3F for a 16 chars and 2 line display
const int buzzerPin = 11;
int trigPin = 6;      // trigger pin
int echoPin = 7;      // echo pin
Adafruit_MLX90614 mlx = Adafruit_MLX90614();


Sd2Card card;
SdVolume volume;
SdFile root;
const int chipSelect = 53;    

struct TagInfo {
  uint8_t uid[7];
  String name;
  int age;
  String contactNumber;
};

TagInfo knownTags[] = {
  {{0x04, 0x77, 0x42, 0xFE, 0xD3, 0x07, 0xC0}, "John Doe", 25, "+1234567890"},
  {{0x04, 0x77, 0x42, 0x5B, 0x96, 0x08, 0xC0}, "Christian Cabrera", 24, "09166810426"},
  {{0x04, 0x77, 0x42, 0x66, 0x62, 0x07, 0xC0}, "Bob Johnson", 22, "+1112223333"}
};


void setup(){
  Serial.begin(9600);
  mlx.begin();
  nfc.begin();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  rtc.halt(false);
  rtc.writeProtect(false);
  // rtc.setDOW(FRIDAY);
  // rtc.setTime(10, 21, 0);
  //h:m:s
  // rtc.setDate(8, 2, 2024);
  //d:m:y
    

  

  Serial.print("\nInitializing SD card...");
  pinMode(53, OUTPUT);
  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card is inserted?");
    Serial.println("* Is your wiring correct?");
    Serial.println("* did you change the chipSelect pin to match your shield or module?");
    return;
  } else {
   Serial.println("Wiring is correct and a card is present."); 
  }
  Serial.print("\nCard type: ");
  switch(card.type()) {
    case SD_CARD_TYPE_SD1:
      Serial.println("SD1");
      break;
    case SD_CARD_TYPE_SD2:
      Serial.println("SD2");
      break;
    case SD_CARD_TYPE_SDHC:
      Serial.println("SDHC");
      break;
    default:
      Serial.println("Unknown");
  }

  // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    return;
  }


  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("\nVolume type is FAT");
  Serial.println(volume.fatType(), DEC);
  Serial.println();
  
  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize *= 512;                            // SD card blocks are always 512 bytes
  Serial.print("Volume size (bytes): ");
  Serial.println(volumesize);
  Serial.print("Volume size (Kbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (Mbytes): ");
  volumesize /= 1024;
  Serial.println(volumesize);

  
  Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  root.openRoot(volume);
  lcd.init();
  lcd.backlight(); 
  lcd.clear();         
  // list all files in the card with date and size
  lcd.setCursor(0,0);
  lcd.print("Reading SD Card.");
  lcd.setCursor(0,1);
  lcd.print("Please wait.");
  root.ls(LS_R | LS_DATE | LS_SIZE);
  lcd.clear();         
  lcd.setCursor(0,0);   //Set cursor to character 2 on line 0
  lcd.print("Welcome...");
  
  lcd.setCursor(0,1);   //Move cursor to character 2 on line 1
  lcd.print("Attendance Monitoring");
  
}

void writeDataToFile(const char *filename, const char *data) {
  SdFile file;

  // Open the file in write mode
  if (file.open(root, filename, FILE_WRITE)) {
    // Write data to the file
    file.println(data);

    // Close the file
    file.close();
    Serial.println("File written successfully!");
  } else {
    Serial.println("Error opening the file.");
  }
}

void loop(){
  
 if (nfc.tagPresent())
{
  NfcTag tag = nfc.read();
  Serial.println(tag.getTagType());
  Serial.print("UID: ");
  Serial.println(tag.getUidString()); // Retrieves the Unique Identification from your tag

  if (tag.hasNdefMessage()) // If your tag has a message
  {
    NdefMessage message = tag.getNdefMessage();
    if (message.getRecordCount() == 0)
    {
      Serial.println("Empty NDEF tag. Not saving to SD card.");
    }
    else
    {
      Serial.print("\nThis Message in this Tag is ");
      Serial.print(message.getRecordCount());
      Serial.print(" NFC Tag Record");
      if (message.getRecordCount() != 1)
      {
        Serial.print("s");
      }
      Serial.println(".");

      // If you have more than 1 Message then it will cycle through them
      for (int i = 0; i < message.getRecordCount(); i++)
      {
        Serial.print("\nNDEF Record ");
        Serial.println(i + 1);
        NdefRecord record = message.getRecord(i);

        int payloadLength = record.getPayloadLength();
        byte payload[payloadLength];
        record.getPayload(payload);

        String payloadAsString = "";
        for (int c = 0; c < payloadLength; c++)
        {
          payloadAsString += (char)payload[c];
        }

          if (payloadAsString.startsWith(" en"))
        {
          payloadAsString = payloadAsString.substring(2);
        }


        beep(1000, 100);

        // if (getUltrasonicDistance() < 10 && payloadAsString.indexOf(',') != -1)
        if (payloadAsString.indexOf(',') != -1)
        {
          float objTemp = mlx.readObjectTempC();
          Serial.print("Object Temperature: ");
          Serial.print(objTemp + 5);
          Serial.println(" °C");

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(payloadAsString);
          lcd.setCursor(0, 1);
          lcd.print("Temp: " + String(objTemp, 1) + " °C");

          writeDataToFile("records.csv", (String(payloadAsString) + "," + String(objTemp) + "," + rtc.getTimeStr() + "," + String(rtc.getDateStr())).c_str());
        }
        else
        {
          Serial.println("Object is too far or Invalid NFC Detected.");
          
        }

        String uid = record.getId();
        if (uid != "")
        {
          Serial.print("  ID: ");
          Serial.println(uid); // Prints the Unique Identification of the NFC Tag
        }
      }
    }
  }
  else
  {
    Serial.println("NFC tag does not have an NDEF message.");
  }
}
else
{
  // teka
  Serial.println("No NFC tag present.");
  if(getUltrasonicDistance() < 50){
      Serial.println("Object detected without tapping NFC. Performing additional actions...");
      beep(1000, 1000);
  }else{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(rtc.getDateStr());
  lcd.setCursor(0,1);
  lcd.print(rtc.getTimeStr());
  // delay(1000);
  }
  
}
}
String formatDate(int day, int month, int year) {
  String formattedDate = "";

  // Add leading zero to day and month if they are less than 10
  formattedDate += (day < 10) ? "0" + String(day) : String(day);
  formattedDate += "-";
  formattedDate += (month < 10) ? "0" + String(month) : String(month);
  formattedDate += "-";
  formattedDate += String(year);

  return formattedDate;
}

float getUltrasonicDistance() {
  // digitalWrite(trigPin, LOW);
    digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  unsigned long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2;

  return distance;
}

bool isNoCardTapped() {
  // You may need to adjust this based on your specific setup
  return digitalRead(echoPin) == HIGH;
}

void beep(int frequency, int duration) {
  tone(buzzerPin, frequency, duration);
  delay(duration); // Delay a bit to prevent cutting off the sound abruptly
  noTone(buzzerPin);
}
