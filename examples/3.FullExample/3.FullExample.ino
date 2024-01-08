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

  Serial.println("FullExample.ino");
  Serial.println("An extensive example of many interesting TinyGPSAsync features");
  Serial.println("(C) Mikal Hart");
  Serial.println();

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
    Serial.println("Fix Qual Sats  HDOP   Latitude   Longitude   Fix Date       Time     Date  Alt   Course  Speed Card  Distance Course Card      Chars   Sentences Checksum Parser"); Serial.flush();
    Serial.println("                      (deg)      (deg)       Age                     Age   (m)   ---- from GPS ----  ---- to London  ----      RX      RX        Fail     Status"); Serial.flush();
    Serial.println("----------------------------------------------------------------------------------------------------------------------------------------------------------------"); Serial.flush();
  }

  auto loc = gps.Location.Get();
  auto date = gps.Date.Get();
  auto time = gps.Time.Get();
  auto course = gps.Course.Get();
  auto distanceKmToLondon = gps.DistanceBetween(loc.Lat, loc.Lng, LONDON_LAT, LONDON_LON) / 1000;
  auto courseToLondon = gps.CourseTo(loc.Lat, loc.Lng, LONDON_LAT, LONDON_LON);
  auto diags = gps.Diagnostic.Get();
  auto fix = gps.FixStatus.Get();
  auto qual = gps.Quality.Get();
  auto satcount = gps.SatelliteCount.Get();
  auto hdop = gps.HDOP.Get();
  auto alt = gps.Altitude.Get();
  auto speed = gps.Speed.Get();
  auto locage = gps.Location.Age();
  auto dateage = gps.Date.Age();
  bool hasFix = fix == 'A';
  string lat = hasFix ? tostring(loc.Lat, 10, 6) : "          ";
  string lng = hasFix ? tostring(loc.Lng, 11, 6) : "           ";
  string londond = hasFix ? tostring(distanceKmToLondon, 8) : "        ";
  string londonc = hasFix ? tostring(courseToLondon, 6, 2) : "      ";
  string londoncd = hasFix ? gps.Cardinal(courseToLondon) : "";
  
  Serial.printf(" %c   %c    %02d   %5.1f %10s %11s  %03s %02d-%02d-%04d %02d:%02d:%02d %03s %7.2f %6.2f %6.2f %4s  %8s %5s %4s   %8d   %9d %8d      %1d\n",
    fix, qual, satcount, hdop, lat.c_str(), lng.c_str(), locage >= 1000 ? "^^^" : tostring(locage, 3).c_str(), date.Day, date.Month, date.Year, time.Hour, time.Minute, time.Second, 
    dateage >= 1000 ? "^^^" : tostring(dateage, 3).c_str(), alt, course, speed, gps.Cardinal(course), londond.c_str(), londonc.c_str(), londoncd.c_str(),
    diags.encodedCharCount, diags.validSentenceCount, diags.failedChecksumCount, gps.Diagnostic.Status());

  delay(1000);
}
