#include <Arduino.h>
#if __has_include("../examples/Device.h")
    #include "../examples/Device.h"
#endif

void setup()
{
  Serial.begin(115200);
  Serial1.begin(GPS_BAUD, SERIAL_8N1, RX, TX);
}

void loop()
{
    if (Serial1.available())
        Serial.write(Serial1.read());
    if (Serial.available())
        Serial1.write(Serial.read());
}