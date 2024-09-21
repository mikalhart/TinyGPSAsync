#include <TinyGPSAsync.h>
#if __has_include("../examples/Device.h")
    #include "../examples/Device.h"
#endif

TinyGPSAsync gps;

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
  if (gps.NewSnapshotAvailable())
  {
    const Snapshot &snapshot = gps.GetSnapshot();

    Serial.printf("%02d:%02d:%02d.%02d %02d-%02d-%04d\n", 
      snapshot.Time.Hour(), snapshot.Time.Minute(), snapshot.Time.Second(), snapshot.Time.Nanos() / 10000000,
      snapshot.Date.Day(), snapshot.Date.Month(), snapshot.Date.Year(),
      snapshot.Location.Lat(), snapshot.Location.Lng());
  }
}
