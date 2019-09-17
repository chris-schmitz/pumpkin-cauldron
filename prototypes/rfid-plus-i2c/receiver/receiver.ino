#include <Wire.h>
#include <Adafruit_NeoPixel.h>

#define FAN 9
#define SPEAKER A0
#define LED_PIN 10
#define LED_COUNT 30

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

uint32_t OFF = strip.Color(0, 0, 0);
uint32_t RED = strip.Color(255, 0, 0);

String pikachu = "460d14ae64c";
String incomingId = "";

void setup()
{
    Serial.begin(9600);

    pinMode(FAN, OUTPUT);
    activateFan();

    strip.begin();
    strip.show();

    fillStrip(RED);
    delay(1000);
    fillStrip(OFF);

    beepSpeaker();

    Wire.begin(8);
    Wire.onReceive(captureTransmission);
}

void loop()
{
    if (incomingId == pikachu)
    {
        Serial.println("Acivating fan");
        activateFan();

        fillStrip(RED);
        beepSpeaker();

        delay(1000);
        fillStrip(OFF);
        incomingId = "";
    }
    delay(100);
}

void captureTransmission(int howMany)
{
    incomingId = "";

    while (1 < Wire.available())
    {
        byte c = Wire.read();
        incomingId.concat(String(c, HEX));
        // Serial.print(c, HEX);
    }

    int x = Wire.read();
    // Serial.println(x);

    Serial.print("Incoming ID: ");
    Serial.println(incomingId);
}

// ! add in seconds parameter
void activateFan()
{
    digitalWrite(FAN, HIGH);
    delay(1000);
    digitalWrite(FAN, LOW);
}

void fillStrip(uint32_t color)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        strip.setPixelColor(i, color);
        strip.show();
        delay(10);
    }
}

void beepSpeaker()
{
    tone(SPEAKER, 400, 100);
    tone(SPEAKER, 400, 100);
}