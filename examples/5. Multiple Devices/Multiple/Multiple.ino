#include <TinyGPSAsync.h>
#if __has_include("../examples/Device.h")
    #include "../examples/Device.h"
#endif

TinyGPSAsync gps1;
TinyGPSAsync gps2;

/* Here's where you customize the second device */
#define GPS_RX2_PIN 3
#define GPS_TX2_PIN (-1)

#define GPS_BAUD2 9600
#define GPS1Serial Serial1
#define GPS2Serial Serial2

void setup()
{
  Serial.begin(115200);
  GPS1Serial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  GPS2Serial.begin(GPS_BAUD2, SERIAL_8N1, GPS_RX2_PIN, GPS_TX2_PIN);

  delay(2000); // Allow ESP32 serial to initialize

  Serial.println("MultipleGPS.ino");
  Serial.println("Demonstrates reading from 2 GPS modules at once");
  Serial.println();

  gps1.begin(GPS1Serial);
  gps2.begin(GPS2Serial);
}

// Displays the time and date from each GPS and (randomly) the latest RMC sentence
void loop()
{
  auto & ss1 = gps1.GetSnapshot();
  auto & ss2 = gps2.GetSnapshot();
  if (ss1.Time.IsNew() || ss2.Time.IsNew())
  {
    auto t1 = ss1.Time;
    auto t2 = ss2.Time;
    auto d1 = ss1.Date;
    auto d2 = ss2.Date;
    Serial.printf("GPS1: %02d:%02d:%02d %02d-%02d-%04d status=%d ", t1.Hour(), t1.Minute(), t1.Second(), d1.Day(), d1.Month(), d1.Year(), gps1.DiagnosticCode());
    Serial.printf("GPS2: %02d:%02d:%02d %02d-%02d-%04d status=%d\n", t2.Hour(), t2.Minute(), t2.Second(), d2.Day(), d2.Month(), d2.Year(), gps2.DiagnosticCode());
    auto & sent1 = gps1.GetSentenceMap();
    auto & sent2 = gps2.GetSentenceMap();
    auto & s1 = sent1["RMC"];
    auto & s2 = sent2["RMC"];
    Serial.printf("%s='%s' ", s1[0].c_str(), s1[1].c_str());
    Serial.printf("%s='%s'\n", s2[0].c_str(), s2[1].c_str());
  }
}
