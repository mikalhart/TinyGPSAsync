#include <TinyGPSAsync.h>
#if __has_include("../examples/Device.h")
    #include "../examples/Device.h"
#endif

#include <map>

TinyGPSAsync gps;

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
  if (gps.NewSatellitesAvailable())
  {
    auto & sats = gps.GetSatellites();
    int inUse = 0;
    for (auto sat : sats)
    {
      if (sat.used) ++inUse;
    }
    Serial.printf("SATS %d (%d): ", sats.size(), inUse);
    string talker = "";

    for (int i=0; i<sats.size(); ++i)
    {
      if (talker != sats[i].talker_id)
      {
        talker = sats[i].talker_id;
        Serial.printf("%s: ", talker.c_str());
      }
      Serial.printf("%3d%c ", sats[i].prn, sats[i].used ? '*' : ' ');
    }
    Serial.println();
  }
}
