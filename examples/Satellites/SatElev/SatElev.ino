#include <Arduino.h>
#include <TinyGPSAsync.h>
#include <map>

TinyGPSAsync gps;

/* Here's where you customize for your personal ESP32 setup */
#define GPS_RX_PIN D0
#define GPS_TX_PIN D1
#define GPS_BAUD 9600
#define GPSSerial Serial1

static const int MAX_SATELLITES = 60;
struct satelevinfo
{
  bool changed = false;
  int elev = -1;
};
std::map<string, vector<satelevinfo>> data;

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

int ElevFromSat(const TinyGPSAsync::Satellites &sats, int satno)
{
  for (int i=0; i<sats.Sats.size(); ++i)
    if (sats.Sats[i].prn == satno)
      return (int)sats.Sats[i].elevation;
  return -1;
}

void Header()
{
  Serial.println();
  Serial.printf("         Id Sat #");
  for (int i=0; i<MAX_SATELLITES; ++i)
    Serial.printf("%02d ", i);
  Serial.println(); Serial.flush();
  Serial.printf("-----------------");
  for (int i=0; i<MAX_SATELLITES; ++i)
    Serial.printf("---");
  Serial.println(); Serial.flush();
}

void loop()
{
  static int linecount = 0;
  bool chg = false;
  auto & sats = gps.GetSatellites();
  if (data.find(sats.Talker) == data.end())
  {
    data[sats.Talker] = vector<satelevinfo>(MAX_SATELLITES);
  }

  for (int i=0; i<MAX_SATELLITES; ++i)
  {
    int elev = ElevFromSat(sats, i);
    auto & thisSat = data[sats.Talker][i];
    if (elev != thisSat.elev)
    {
      thisSat.changed = chg = true;
      thisSat.elev = elev;
    }
  }
  
  if (chg)
  {
    if (linecount++ % 20 == 0)
      Header();
    auto t = gps.GetSnapshot().Time;
    Serial.printf("%02d:%02d:%02d %2s      ", t.Hour(), t.Minute(), t.Second(), sats.Talker.c_str());
    for (int i=0; i<MAX_SATELLITES; ++i)
    {
      auto & thisSat = data[sats.Talker][i];
      char ch = thisSat.changed && linecount != 1 ? '*' : ' ';
      if (thisSat.elev == -1)
        Serial.printf("  %c", ch);
      else
        Serial.printf("%02d%c", thisSat.elev, ch);
      thisSat.changed = false;
    }
    Serial.println();
  }
}
