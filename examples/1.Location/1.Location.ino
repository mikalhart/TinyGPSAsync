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

  delay(1500); // Allow ESP32 serial to initialize

  Serial.println("Location.ino");
  Serial.println("TinyGPSAsync example for retrieving location");
  Serial.println("(C) Mikal Hart");
  Serial.println();

  gps.begin(GPSSerial);
}

void loop()
{
  if (gps.Location.IsNew())
  {
    auto l = gps.Location.Get();
    Serial.printf("New location: (%f, %f)\n", l.Lat, l.Lng);
  }

  int parserStatus = gps.Diagnostic.Status();
  if (parserStatus != 0)
  {
    Serial.printf("Diagnostic: %s\n", gps.Diagnostic.StatusString(parserStatus).c_str());
    delay(1000);
  }
}