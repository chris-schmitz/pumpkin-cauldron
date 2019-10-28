#include <SPI.h>
#include <MFRC522.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_VS1053.h>
#include <SD.h>

#define DEBUG false
#define USING_MUSIC_MAKER true

// ^ Music Maker settings
#define VS1053_RESET -1 // VS1053 reset pin (not used!)

#define VS1053_DCS 10 // VS1053 Data/command select pin (output)
#define VS1053_DREQ 9 // VS1053 Data request, ideally an Interrupt pin
#define VS1053_CS 6   // VS1053 chip select pin (output)
#define CARDCS 5      // Card chip select pin

#define VOLUME 0

Adafruit_VS1053_FilePlayer musicPlayer =
    Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

// ^ RFID settings
#define RST_PIN 18
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
#define FAN 17

enum FAN_STATUSES
{
    FAN_OFF = 0,
    FAN_ON = 1
};

// | Tags
String SQUIRTLE = " 04 59 d0 4a e6 4c 81";
String PIKACHU = " 04 60 d1 4a e6 4c 81";
String CUPCAKE = " 04 c8 cd 4a e6 4c 80";

String content;
boolean ledsOn = true;
boolean effectsActive = false;

void setup()
{
    Serial.begin(115200);

    if (DEBUG)
    {
        while (!Serial)
            ;
    }

    if (USING_MUSIC_MAKER)
    {
        setupMusicMaker();
    }

    // ^ LED Strip setup
    strip.begin();
    strip.show();

    // ^ RFID setup
    SPI.begin();
    mfrc522.PCD_Init();
    mfrc522.PCD_DumpVersionToSerial();

    if (DEBUG)
    {
        Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
    }

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
    LIGHT_PATTERN_WITCH_CACKLE,
    LIGHT_PATTERN_EVIL_LAUGH
};

uint8_t activePattern = LIGHT_PATTERN_BUBBLE;

void loop()
{

    // ^ Advance the state machines
    runActiveLightPattern();

    // ^ Look for new RFID tags
    if (!mfrc522.PICC_IsNewCardPresent())
    {
        return;
    }

    // ^ Select one of the tags
    if (!mfrc522.PICC_ReadCardSerial())
    {
        return;
    }

    captureUID();
    // return;

    // ^ If we're running effects we don't need to set anything
    // ! Note that nothing that needs to be run each loop pass (e.g. state machine driven
    // ! code) should be put after this conditional
    if (effectsActive)
    {
        return;
    }

    // ^ Prep a new run of the effects
    effectsActive = true;

    if (USING_MUSIC_MAKER)
    {
        musicPlayer.stopPlaying();
    }

    // TODO: Break each conditional out into it's own named function
    // ^ Run the specific set of effects based on the RFID tag we captured
    if (content == PIKACHU)
    {
        setFanStatus(FAN_ON);
        if (USING_MUSIC_MAKER)
        {
            musicPlayer.startPlayingFile("SOUNDS/EFFECTS/TREX.MP3");
        }
        setActiveLightPattern(LIGHT_PATTERN_MONSTER_ROAR);
    }
    else if (content == SQUIRTLE)
    {
        setFanStatus(FAN_ON);
        if (USING_MUSIC_MAKER)
        {
            musicPlayer.startPlayingFile("SOUNDS/EFFECTS/WITCH2.MP3");
        }
        setActiveLightPattern(LIGHT_PATTERN_WITCH_CACKLE);
    }
    else if (content == CUPCAKE)
    {
        Serial.println("Got the cupcake!");
        setFanStatus(FAN_ON);
        if (USING_MUSIC_MAKER)
        {
            musicPlayer.startPlayingFile("SOUNDS/EFFECTS/LAUGH3.MP3");
            // musicPlayer.startPlayingFile("SOUNDS/PARADO/PC-TY.MP3");
        }
        setActiveLightPattern(LIGHT_PATTERN_WITCH_CACKLE);
    }
    // | Verified Good
    //  musicPlayer.startPlayingFile("SOUNDS/EFFECTS/WITCH1.MP3");
    // musicPlayer.startPlayingFile("SOUNDS/EFFECTS/BUBBLE.MP3");
}

void setupMusicMaker()
{
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

    printDirectory(SD.open("/SOUNDS/"), 0);

    // ^ Add settings for the music maker and test
    if (USING_MUSIC_MAKER)
    {
        musicPlayer.setVolume(VOLUME, VOLUME);
        musicPlayer.useInterrupt(VS1053_FILEPLAYER_PIN_INT); // DREQ int
                                                             // musicPlayer.playFullFile("SOUNDS/PARADO/HI-THERE.MP3");
    }
}

// TODO: RENAME
// ! hmmm if we're going to deactivate the fan in here based on the music player status
// ! we should really rename this function
void runActiveLightPattern()
{
    if (USING_MUSIC_MAKER)
    {

        if (musicPlayer.stopped())
        {
            setActiveLightPattern(LIGHT_PATTERN_BUBBLE);
            setFanStatus(FAN_OFF);
            effectsActive = false;
        }
    }
    // | Note! we're intentionally using if else if statements here instead of a switch for speed purposes.
    // | If we use switch, each case has to be evaluated even if the first one is the only one we're looking for,
    // | but if we use an if/else if then we'll stop once we find our desired conditional.
    if (activePattern == LIGHT_PATTERN_BUBBLE)
    {
        lightsBubble();
    }
    else if (activePattern == LIGHT_PATTERN_MONSTER_ROAR)
    {
        lightsMonsterRoar();
    }
    else if (activePattern == LIGHT_PATTERN_WITCH_CACKLE)
    {
        lightsWitchesCackle();
    }
    else if (activePattern == LIGHT_PATTERN_EVIL_LAUGH)
    {
        lightsEvilLaugh();
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

unsigned long prevMillisLAUGH = 0;
unsigned int intervalLAUGH = 10;
uint8_t colorIndex = 0;
const uint32_t laugh_array[9] = {GREEN_DARK, BLUE_DARK, RED_DARK, GREEN_MEDIUM, BLUE_MEDIUM, RED_MEDIUM, GREEN_LIGHT, BLUE_LIGHT, RED_LIGHT};

void lightsEvilLaugh()
{
    unsigned long currentMillis = millis();

    if (currentMillis - prevMillisLAUGH > intervalLAUGH)
    {
        for (uint8_t i = 0; i < TOTAL_LEDS; i++)
        {
            strip.setPixelColor(i, laugh_array[colorIndex]);
            strip.show();
        }
        prevMillisLAUGH = currentMillis;
    }

    colorIndex++;
    if (colorIndex > 8)
    {
        colorIndex = 0;
    }
}
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
    if (DEBUG)
    {
        Serial.println(content);
    }
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
    if (DEBUG && fanStatus == 1)
    {
        Serial.print("Fan status: ");
        Serial.println(fanStatus);
    }
    digitalWrite(FAN, fanStatus);
}
