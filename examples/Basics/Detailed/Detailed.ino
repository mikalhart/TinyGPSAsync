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

  Serial.println("FullExample.ino");
  Serial.println("An extensive example of many interesting TinyGPSAsync features");
  Serial.println();

  delay(3000);
  gps.begin(GPSSerial);
}

string tostring(uint32_t n, int digs)
{
  char buf[digs + 10];
  sprintf(buf, "%*d", digs, n);
  return buf;
}

string tostring(double n, int digs, int ddigs)
{
  char buf[digs + 10];
  sprintf(buf, "%*.*f", digs, ddigs, n);
  return buf;
}

void loop()
{
  static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;
  static int count = 0;

  if (count++ % 20 == 0)
  {
    Serial.println(); Serial.flush();
    Serial.println("Fix Qual Sats   HDOP  Latitude   Longitude   Fix Date       Time     Date  Alt   Course  Speed Card  Distance Course Card      Chars   Sentences Checksum   - Status -"); Serial.flush();
    Serial.println("                         (deg)       (deg)   Age                     Age   (m)   ---- from GPS ----  ---- to London  ----         RX          RX     Fail   Code String"); Serial.flush();
    Serial.println("----------------------------------------------------------------------------------------------------------------------------------------------------------------------------"); Serial.flush();
  }

  auto & snapshot = gps.GetSnapshot();
  auto loc = snapshot.Location;
  auto date = snapshot.Date;
  auto time = snapshot.Time;
  auto distanceKmToLondon = gps.DistanceBetween(loc.Lat.Degrees(), loc.Lng.Degrees(), LONDON_LAT, LONDON_LON) / 1000;
  auto courseToLondon = gps.CourseTo(loc.Lat.Degrees(), loc.Lng.Degrees(), LONDON_LAT, LONDON_LON);
  auto stats = gps.GetStatistics();
  auto fix = snapshot.FixStatus.Value();
  auto qual = snapshot.Quality.Value();
  auto satcount = snapshot.SatelliteCount.Value();
  auto hdop = snapshot.HDOP.AsDouble();
  auto alt = snapshot.Altitude.Meters();
  auto course = snapshot.Course.Degrees();
  auto speed = snapshot.Speed.Mps();
  auto locage = snapshot.Location.Age();
  auto dateage = snapshot.Date.Age();
  bool hasFix = snapshot.FixStatus.IsPositionValid();
  string lat = hasFix ? tostring(loc.Lat.Degrees(), 10, 6) : "          ";
  string lng = hasFix ? tostring(loc.Lng.Degrees(), 11, 6) : "           ";
  string londond = hasFix ? tostring(distanceKmToLondon, 8) : "        ";
  string londonc = hasFix ? tostring(courseToLondon, 6, 2) : "      ";
  string londoncd = hasFix ? gps.Cardinal(courseToLondon) : "";
  string altstr = snapshot.Altitude.Age() < 2000 ? tostring(snapshot.Altitude.Meters(), 7, 2) : "       ";
  string crsstr = snapshot.Course.Age() < 2000 ? tostring(snapshot.Course.Degrees(), 6, 2) : "      ";
  string spdstr = snapshot.Speed.Age() < 2000 ? tostring(snapshot.Speed.Mps(), 6, 2) : "      ";
    
  Serial.printf(" %c   %c    %02d   %5.1f %10s %11s  %03s %02d-%02d-%04d %02d:%02d:%02d %03s %7s %6s %6s %4s  %8s %5s %4s   %8d   %9d %8d      %1d %s\n",
    fix, qual, satcount, hdop, lat.c_str(), lng.c_str(), locage >= 1000 ? "^^^" : tostring(locage, 3).c_str(), date.Day(), date.Month(), date.Year(), time.Hour(), time.Minute(), time.Second(),
        dateage >= 1000 ? "^^^" : tostring(dateage, 3).c_str(), altstr.c_str(), crsstr.c_str(), spdstr.c_str(), gps.Cardinal(course), londond.c_str(), londonc.c_str(), londoncd.c_str(),
    stats.encodedCharCount, stats.validSentenceCount, stats.failedChecksumCount, gps.DiagnosticCode(), gps.DiagnosticString().c_str());
  delay(1000);
}
