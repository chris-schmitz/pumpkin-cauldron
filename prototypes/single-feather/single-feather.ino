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

#define VOLUME 5

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
// | NOTE: this include needs to happen after strip is defined so that we have access to
// | the `strip` object.
#include "src/colors.h"

// ^ Fan settings
#define FAN 16

enum FAN_STATUSES
{
    FAN_OFF = 0,
    FAN_ON = 1
};

// | Tags
String SQUIRTLE = " 04 59 d0 4a e6 4c 81";
String PIKACHU = " 04 60 d1 4a e6 4c 81";

boolean ledsOn = true;

String content;

void setup()
{
    Serial.begin(115200);

    // while (!Serial)
    //     ;

    // ^ Music maker setup
    if (!musicPlayer.begin())
    {
        Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
        while (1)
            ;
    }
    Serial.println(F("VS1053 found"));

    if (!SD.begin(CARDCS))
    {
        Serial.println(F("SD failed, or not present"));
        while (1)
            ;
    }

    // printDirectory(SD.open("/SOUNDS/"), 0);

    // ^ Add settings for the music maker and test
    musicPlayer.setVolume(VOLUME, VOLUME);
    musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int
    // musicPlayer.playFullFile("SOUNDS/PARADO/HI-THERE.MP3");

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
    setFanStatus(FAN_ON);
    delay(2000);
    setFanStatus(FAN_OFF);
}

enum LIGHT_PATTERNS
{
    LIGHT_PATTERN_BUBBLE = 0,
    LIGHT_PATTERN_MONSTER_ROAR,
    LIGHT_PATTERN_WITCH_CACKLE
};

uint8_t activePattern = LIGHT_PATTERN_BUBBLE;

void loop()
{
    // ? Left in for testing out each color group
    // fillStrip(BLUE_DARK);
    // delay(1000);
    // fillStrip(BLUE_MEDIUM);
    // delay(1000);
    // fillStrip(BLUE_LIGHT);
    // delay(1000);
    // return;

    runActiveLightPattern();

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

    captureUID();

    musicPlayer.stopPlaying();

    if (content == PIKACHU)
    {
        setFanStatus(FAN_ON);
        musicPlayer.startPlayingFile("SOUNDS/EFFECTS/TREX.MP3");
        setActiveLightPattern(LIGHT_PATTERN_MONSTER_ROAR);
    }
    else if (content == SQUIRTLE)
    {
        setFanStatus(FAN_ON);
        musicPlayer.startPlayingFile("SOUNDS/EFFECTS/WITCH2.MP3");
        fillStrip(CYAN);
    }
    // | Verified Good
    //  musicPlayer.startPlayingFile("SOUNDS/EFFECTS/WITCH1.MP3");
    // musicPlayer.startPlayingFile("SOUNDS/EFFECTS/BUBBLE.MP3");
}

// TODO: RENAME
// ! hmmm if we're going to deactivate the fan in here based on the music player status
// ! we should really rename this function
void runActiveLightPattern()
{
    // | Note! we're intentionally using if else if statements here instead of a switch for speed purposes.
    // | If we use switch, each case has to be evaluated even if the first one is the only one we're looking for,
    // | but if we use an if/else if then we'll stop once we find our desired conditional.
    if (activePattern == LIGHT_PATTERN_BUBBLE)
    {
        lightsBubble();
    }
    else if (activePattern == LIGHT_PATTERN_MONSTER_ROAR)
    {
        if (musicPlayer.stopped())
        {
            setActiveLightPattern(LIGHT_PATTERN_BUBBLE);
            setFanStatus(FAN_OFF);
        }
        lightsMonsterRoar();
    }
}

unsigned long previousMillis = 0;
unsigned long lightChangeInterval = 100;

const uint32_t lightsBubbleArray[5] = {PURPLE_DARK, PURPLE_LIGHT, PURPLE_MEDIUM, BLUE_DARK, GREEN_DARK};
/**
 * |Slow purple and blue pulse pattern
 **/
void lightsBubble()
{
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis > lightChangeInterval)
    {
        for (uint8_t i = 0; i < TOTAL_LEDS; i++)
        {
            uint32_t randomIndex = rand() % 5;
            strip.setPixelColor(i, lightsBubbleArray[randomIndex]);
        }
        strip.show();

        previousMillis = currentMillis;
    }
}

uint8_t flashOffset = 0;
/**
 * | Flashing yellow and red
 **/
void lightsWitchesCackle()
{
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis > lightChangeInterval)
    {
        for (uint8_t i = 0; i < TOTAL_LEDS; i++)
        {
            if (flashOffset + i % 2 == 0)
            {
                strip.setPixelColor(i, RED_LIGHT);
            }
            else
            {
                strip.setPixelColor(i, YELLOW_LIGHT);
            }
        }
        strip.show();

        // TODO come back and do boolean casting here or up in the if to simplify this
        if (flashOffset == 1)
        {
            flashOffset = 0;
        }
        else
        {
            flashOffset = 1;
        }

        previousMillis = currentMillis;
    }
}

bool lightFlashToggle = true;
unsigned long previousMillisMONSTER = 0;
unsigned int lightChangeIntervalMONSTER = 10;

/**
 * | Flashing red off and on
 * 
 * * Note that we do not need to track duration here. This routine will stop running 
 * * once the music player is done playing the audio file.
 **/
void lightsMonsterRoar()
{
    unsigned long currentMillis = millis();

    if (currentMillis - previousMillisMONSTER > lightChangeIntervalMONSTER)
    {
        for (uint8_t i = 0; i < TOTAL_LEDS; i++)
        {
            if (lightFlashToggle)
            {
                strip.setPixelColor(i, RED_LIGHT);
            }
            else
            {
                strip.setPixelColor(i, YELLOW_DARK);
            }
            strip.show();
            lightFlashToggle = !lightFlashToggle;
        }

        lightFlashToggle = !lightFlashToggle;

        previousMillisMONSTER = currentMillis;
    }
}
void lightsEvilLaugh() {}
void lightsDogBark() {}

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

void setActiveLightPattern(int pattern)
{
    activePattern = pattern;
}

void setFanStatus(uint8_t fanStatus)
{
    // Serial.print("Fan status: ");
    // Serial.println(fanStatus);
    digitalWrite(FAN, fanStatus);
}