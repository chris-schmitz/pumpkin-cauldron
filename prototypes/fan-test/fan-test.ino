#define FAN 17

void setup()
{
    Serial.begin(9600);
    pinMode(FAN, OUTPUT);
}

void loop()
{
    activateFan();
    delay(2000);
}

void activateFan()
{
    Serial.println("Fan ON");
    digitalWrite(FAN, HIGH);
    delay(5000);
    digitalWrite(FAN, LOW);
    Serial.println("Fan OFF");
    delay(5000);
}