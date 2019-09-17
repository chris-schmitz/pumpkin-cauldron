#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>

// ^ RFID settings
#define RST_PIN 15
#define SS_PIN 19
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ^ i2c settings
byte x = 0;
String content;

void setup()
{
    Serial.begin(9600); // Initialize serial communications with the PC

    // ^ i2c setup
    Wire.begin();

    // ^ RFID setup
    // while (!Serial)
    //     ;
    SPI.begin();
    mfrc522.PCD_Init();
    mfrc522.PCD_DumpVersionToSerial();
    Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
}

void loop()
{

    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return;
    }

    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial())
    {
        return;
    }

    // // Dump debug info about the card; PICC_HaltA() is automatically called
    // mfrc522.PICC_DumpToSerial(&(mfrc522.uid));

    captureUID();

    delay(100);
}

void captureUID()
{
    content = "";
    Wire.beginTransmission(8);

    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
        Wire.write(mfrc522.uid.uidByte[i]);
    }
    Serial.println(content);
    Wire.endTransmission();
}
