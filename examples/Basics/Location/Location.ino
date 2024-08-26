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

  delay(2000); // Allow ESP32 serial to initialize

  Serial.println("Location.ino");
  Serial.println("TinyGPSAsync example for retrieving location");
  Serial.println();

  gps.begin(GPSSerial);
}

void loop()
{
  if (gps.NewSnapshotAvailable())
  {
    auto &ss = gps.GetSnapshot();
    auto &st = gps.GetSentences();
    bool locnew = ss.Location.IsNew();
    bool timenew = ss.Time.IsNew();
    if (locnew || timenew)
      Serial.printf("%c(%f, %f) %c%02d:%02d:%02d %s\n", locnew ? '*' : ' ', ss.Location.Lat(), ss.Location.Lng(), timenew ? '*' : ' ', ss.Time.Hour(), ss.Time.Minute(), ss.Time.Second(), st.LastSentence.String().c_str());
  }

  int parserStatus = gps.DiagnosticCode();
  if (parserStatus != 0)
  {
    Serial.printf("Diagnostic: %s\n", gps.DiagnosticString().c_str());
    delay(1000);
  }
}