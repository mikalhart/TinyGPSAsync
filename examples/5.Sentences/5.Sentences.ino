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

  Serial.println("Sentences.ino");
  Serial.println("Looking at raw NMEA sentences");
  Serial.println("by Mikal Hart");
  Serial.println();

  gps.begin(GPSSerial);
}

// This code displays all the Sentence Ids that have been received,
// and then the latest GGA sentence
void loop()
{
    auto sentences = gps.Sentence.Get();
    Serial.print("Sentence IDs seen: ");
    for (auto s:sentences)
        Serial.printf("%s ", s.second.SentenceId().c_str());
    Serial.println();
    auto gga = gps.Sentence["GGA"].Get();
    Serial.printf("Latest GGA sentence: %s\n", gga.c_str());
    delay(5000);
}
