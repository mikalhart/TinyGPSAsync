#include <TinyGPSAsync.h>
#if __has_include("../examples/Device.h")
    #include "../examples/Device.h"
#endif

TinyGPSAsync gps;

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
    auto &snapshot = gps.GetSnapshot();
    auto &packet = gps.GetLatestPacket();
    bool locnew = snapshot.Location.IsNew();
    bool timenew = snapshot.Time.IsNew();
    if (locnew || timenew)
      Serial.printf("%c(%f, %f) %c%02d:%02d:%02d %s\n", locnew ? '*' : ' ', snapshot.Location.Lat(), snapshot.Location.Lng(), timenew ? '*' : ' ', snapshot.Time.Hour(), snapshot.Time.Minute(), snapshot.Time.Second(), packet.ToString().c_str());
  }

  int parserStatus = gps.DiagnosticCode();
  if (parserStatus != 0)
  {
    Serial.printf("Diagnostic: %s\n", gps.DiagnosticString());
    delay(1000);
  }
}