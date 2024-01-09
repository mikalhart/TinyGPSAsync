#include <Arduino.h>
#include <TinyGPSAsync.h>

TinyGPSAsync gps;

/* Here's where you customize for your personal ESP32 setup */
#define GPS_RX_PIN 2
#define GPS_TX_PIN 3
#define GPS_BAUD 9600
#define GPSSerial Serial1

void setup()
{
  Serial.begin(115200);
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  delay(2000); // Allow ESP32 serial to initialize

  Serial.println("DateTime.ino");
  Serial.println("TinyGPSAsync example for retrieving date and time");
  Serial.println();

  gps.begin(GPSSerial);
}

void loop()
{
  if (gps.Time.IsNew())
  {
    auto t = gps.Time.Get();
    Serial.printf("%02d:%02d:%02d ", t.Hour, t.Minute, t.Second);
    if (gps.Date.EverUpdated())
    {
        auto d = gps.Date.Get();
        Serial.printf("%02d-%02d-%04d\n", d.Day, d.Month, d.Year);
    }
    else
    {
        Serial.printf("[Date unknown]\n");
    }
  }
}
