#include <Esp.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 19 // pin number is specific to your esp32 board
#define LED_BUILTIN_IS_REVERSE false
#else
#define LED_BUILTIN_IS_REVERSE true
#endif

class Led
{
public:
    Led();
    Led(int ledPin);
    Led(int ledPin, bool isReverse);
    ~Led();
    void on();
    void off();
    void setIsReverse(bool isReverse);
    int getStatus();
    int getLedPin();
    void setLedPin(int ledPin);

private:
    int ledPin = LED_BUILTIN;
    bool isReverse =  false;
};

Led::Led()
{
    this->setIsReverse(LED_BUILTIN_IS_REVERSE);
    pinMode(ledPin, OUTPUT);
}

Led::Led(int ledPin)
{
    this->ledPin = ledPin;
    pinMode(ledPin, OUTPUT);
}

Led::Led(int ledPin, bool isReverse)
{
    this->ledPin = ledPin;
    this->setIsReverse(isReverse);
}

Led::~Led(){}

void Led::on()
{
    if (isReverse){
        digitalWrite(ledPin, LOW);
    } else {
        digitalWrite(ledPin, HIGH);
    }
}

void Led::off()
{
    if (isReverse){
        digitalWrite(ledPin, HIGH);
    } else {
       digitalWrite(ledPin, LOW);
    }
}

void Led::setIsReverse(bool isReverse)
{
    this->isReverse = isReverse;
}


int Led::getStatus()
{
    if (isReverse){
        return !digitalRead(ledPin);
    } else {
        return digitalRead(ledPin);
    }
}

int Led::getLedPin()
{
    return ledPin;
}

void Led::setLedPin(int ledPin)
{
    this->ledPin = ledPin;
    pinMode(ledPin, OUTPUT);
}