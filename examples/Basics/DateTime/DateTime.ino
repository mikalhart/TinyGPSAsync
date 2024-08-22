#include <Arduino.h>
#include <TinyGPSAsync.h>

TinyGPSAsync gps;

/* Here's where you customize for your personal ESP32 setup */
#define GPS_RX_PIN D0
#define GPS_TX_PIN D1
#define GPS_BAUD 9600
#define GPSSerial Serial1

void setup()
{
  Serial.begin(115200);
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  delay(3000); // Allow ESP32 serial to initialize

  Serial.println("DateTime.ino");
  Serial.println("TinyGPSAsync example for retrieving date and time");
  Serial.println();

  gps.begin(GPSSerial);
}

void loop()
{
  if (gps.NewSentenceAvailable())
  {
    bool a = gps.NewSnapshotAvailable();
    bool b = gps.NewSatellitesAvailable();
    bool c = gps.NewSentenceAvailable();
    bool d = gps.NewCharactersAvailable();
    const TinyGPSAsync::Snapshot &snapshot = gps.GetSnapshot();
    const TinyGPSAsync::Sentences &sentences = snapshot.sentences;
    Serial.printf("%d:%d:%d:%d %02d:%02d:%02d.%02d %02d-%02d-%04d (%.6f, %.6f) %s\n", a, b, c, d,
      snapshot.Time.Hour(), snapshot.Time.Minute(), snapshot.Time.Second(), snapshot.Time.Centisecond(),
      snapshot.Date.Day(), snapshot.Date.Month(), snapshot.Date.Year(),
      snapshot.Location.Lat(), snapshot.Location.Lng(),
      sentences.LastSentence.String().c_str());
  }
}
