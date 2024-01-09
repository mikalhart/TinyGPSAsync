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

  Serial.println("Diagnostics.ino");
  Serial.println("Quick troubleshooting in TinyGPSAsync");
  Serial.println();

  gps.begin(GPSSerial);
}

void loop()
{
  int status = gps.Diagnostic.Status();
  string statusString = gps.Diagnostic.StatusString(status);
  auto counters = gps.Diagnostic.Get();
  Serial.printf("Characters Rx = %d, Sentences Valid/Invalid = %d/%d, Chksum pass/fail = %d/%d, Status = '%s' (%d)\n",
    counters.encodedCharCount, counters.validSentenceCount, counters.invalidSentenceCount, counters.passedChecksumCount, counters.failedChecksumCount,
    statusString.c_str(), status);

  delay(1000);
}
