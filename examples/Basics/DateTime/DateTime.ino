#include <Arduino.h>
#include <TinyGPSAsync.h>

TinyGPSAsync gps;

/* Here's where you customize for your personal ESP32 setup */
#define GPS_RX_PIN D1
#define GPS_TX_PIN D0
#define GPS_BAUD 115200
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
  bool newSnap = gps.NewSnapshotAvailable();
  bool newSent = gps.NewSentenceAvailable();
  bool newPacket = gps.NewUbxPacketAvailable();
  if (newSent || newSnap || newPacket)
  {
    const Snapshot &snapshot = gps.GetSnapshot();
    const Sentences &sentences = gps.GetSentences();
    const UbxPackets &packets = gps.GetUbxPackets();
    if (newSent)
    {
      const ParsedSentence &latest = sentences.LastSentence;
      if (latest.SentenceId() != "GGA" && latest.SentenceId() != "RMC")
        newSent = false;
    }

    if (newPacket)
    {
      const ParsedUbxPacket &latest = packets.LastUbxPacket;
      if (latest.Id() != 7)
        newPacket = false;
    }


    if (newSent || newSnap || newPacket)
      Serial.printf("%02d:%02d:%02d.%2d %02d-%02d-%04d (%.6f, %.6f) %s\n", 
        snapshot.Time.Hour(), snapshot.Time.Minute(), snapshot.Time.Second(), snapshot.Time.Nanos() / 10000000,
        snapshot.Date.Day(), snapshot.Date.Month(), snapshot.Date.Year(),
        snapshot.Location.Lat(), snapshot.Location.Lng(),
        newSent ? sentences.LastSentence.ToString().c_str() : packets.LastUbxPacket.ToString().c_str());
  }
}
