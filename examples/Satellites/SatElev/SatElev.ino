#include <Arduino.h>
#include <TinyGPSAsync.h>

TinyGPSAsync gps;

/* Here's where you customize for your personal ESP32 setup */
#define GPS_RX_PIN 2
#define GPS_TX_PIN 3
#define GPS_BAUD 9600
#define GPSSerial Serial1

static const int MAX_SATELLITES = 40;
int elevations[MAX_SATELLITES];
bool changed[MAX_SATELLITES];

void setup()
{
  Serial.begin(115200);
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  delay(2000); // Allow ESP32 serial to initialize

  Serial.println("SatElev.ino");
  Serial.println("Examining GSV satellite data");
  Serial.println();

  gps.begin(GPSSerial);
}

int ElevFromSat(std::vector<TinyGPSAsync::SatelliteItem::SatInfo> &sats, int satno)
{
  for (int i=0; i<sats.size(); ++i)
    if (sats[i].prn == satno)
      return (int)sats[i].elevation;
  return -1;
}

void Header()
{
  Serial.println();
  Serial.printf("     Sat #");
  for (int i=0; i<MAX_SATELLITES; ++i)
    Serial.printf("%02d ", i);
  Serial.println(); Serial.flush();
  Serial.printf("----------");
  for (int i=0; i<MAX_SATELLITES; ++i)
    Serial.printf("---");
  Serial.println(); Serial.flush();
}

void loop()
{
  if (gps.Satellites.IsNew())
  {
    static int linecount = 0;
    bool chg = false;
    auto sats = gps.Satellites.Get();
    for (int i=0; i<MAX_SATELLITES; ++i)
    {
      int elev = ElevFromSat(sats, i);
      if (elev != elevations[i])
      {
        changed[i] = chg = true;
        elevations[i] = elev;
      }
    }
    
    if (chg)
    {
      if (linecount++ % 20 == 0)
        Header();
      auto t = gps.Time.Get();
      Serial.printf("%02d:%02d:%02d  ", t.Hour, t.Minute, t.Second, t.Centisecond);
      for (int i=0; i<MAX_SATELLITES; ++i)
      {
        char ch = changed[i] && linecount != 1 ? '*' : ' ';
        if (elevations[i] == -1)
          Serial.printf("  %c", ch);
        else
          Serial.printf("%02d%c", elevations[i], ch);
        changed[i] = false;
      }
      Serial.println();
    }
  }
}
