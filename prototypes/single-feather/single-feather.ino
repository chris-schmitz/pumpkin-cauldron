#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

// ^ Music Maker settings
#define VS1053_RESET -1 // VS1053 reset pin (not used!)

#define VS1053_DCS 10 // VS1053 Data/command select pin (output)
#define VS1053_DREQ 9 // VS1053 Data request, ideally an Interrupt pin
#define VS1053_CS 6   // VS1053 chip select pin (output)
#define CARDCS 5      // Card chip select pin

Adafruit_VS1053_FilePlayer musicPlayer =
    Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

// ^ RFID settings
#define RST_PIN 15
#define SS_PIN 19
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ^ LED strip settings
#define LED_PIN 12
#define TOTAL_LEDS 30
Adafruit_NeoPixel strip = Adafruit_NeoPixel(TOTAL_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// ^ Fan settings
#define FAN 17
enum FanStatus
{
    FAN_ON,
    FAN_OFF
};

// | Tags
String SQUIRTLE = " 04 59 d0 4a e6 4c 81";
String PIKACHU = " 04 60 d1 4a e6 4c 81";

// | Colors
uint32_t OFF = strip.Color(0, 0, 0);
uint32_t CYAN = strip.Color(0, 255, 255);
uint32_t YELLOW = strip.Color(255, 255, 0);
boolean ledsOn = true;

String content;

void setup()
{
    Serial.begin(115200);
    while (!Serial)
        ;

    // ^ Music maker setup
    if (!musicPlayer.begin())
    { // initialise the music player
        Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
        while (1)
            ;
    }
    Serial.println(F("VS1053 found"));

    if (!SD.begin(CARDCS))
    {
        Serial.println(F("SD failed, or not present"));
        while (1)
            ; // don't do anything more
    }

    // list files
    printDirectory(SD.open("/"), 0);

    // Set volume for left, right channels. lower numbers == louder volume!
    musicPlayer.setVolume(0, 0);
    musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int
    musicPlayer.playFullFile("PARADO/HI-THERE.MP3");

    // ^ LED Strip setup
    strip.begin();
    strip.show();

    // ^ RFID setup
    SPI.begin();
    mfrc522.PCD_Init();
    mfrc522.PCD_DumpVersionToSerial();
    Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

    // ^ Fan setup
    pinMode(FAN, OUTPUT);
    digitalWrite(FAN, HIGH);
    delay(2000);
    digitalWrite(FAN, LOW);
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

    if (content == PIKACHU)
    {
        Serial.println("Starting playback");
        musicPlayer.startPlayingFile("EFFECTS/LION.WAV");
        setFanStatus(FAN_ON);
        Serial.println("Finished playback");
        fillStrip(YELLOW);
        setFanStatus(FAN_OFF);
    }
    else if (content == SQUIRTLE)
    {
        musicPlayer.startPlayingFile("EFFECTS/LAUGH.WAV");
        setFanStatus(FAN_ON);
        fillStrip(CYAN);
        setFanStatus(FAN_OFF);
    }

    delay(2000);
    fillStrip(OFF);

    delay(100);
}

void setFanStatus(FanStatus status)
{
    if (status == FAN_ON)
    {
        digitalWrite(FAN, HIGH);
    }
    else
    {
        digitalWrite(FAN, LOW);
    }
}

// ! switch this logic to state machine to remove the blocking nature
void fillStrip(uint32_t color)
{
    for (uint8_t i = 0; i < TOTAL_LEDS; i++)
    {
        strip.setPixelColor(i, color);
        strip.show();
        delay(10);
    }
}

void captureUID()
{
    content = "";

    for (byte i = 0; i < mfrc522.uid.size; i++)
    {
        content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }
    Serial.println(content);
}

void printDirectory(File dir, int numTabs)
{
    while (true)
    {

        File entry = dir.openNextFile();
        if (!entry)
        {
            // no more files
            //Serial.println("**nomorefiles**");
            break;
        }
        for (uint8_t i = 0; i < numTabs; i++)
        {
            Serial.print('\t');
        }
        Serial.print(entry.name());
        if (entry.isDirectory())
        {
            Serial.println("/");
            printDirectory(entry, numTabs + 1);
        }
        else
        {
            // files have sizes, directories do not
            Serial.print("\t\t");
            Serial.println(entry.size(), DEC);
        }
        entry.close();
    }
}
