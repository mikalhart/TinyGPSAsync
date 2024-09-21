#include <TinyGPSAsync.h>
#if __has_include("../examples/Device.h")
    #include "../examples/Device.h"
#endif

#include <map>

TinyGPSAsync gps;

void setup()
{
  Serial.begin(115200);
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  delay(3000); // Allow ESP32 serial to initialize

  Serial.println("SatElev.ino");
  Serial.println("Examining GSV satellite data");
  Serial.println();
  delay(1000);

  gps.begin(GPSSerial);
  UBXProtocol ubx(Serial1);
  ubx.reset(UBXProtocol::RESET_COLD);
  delay(1000);
  ubx.setpacketrate(UBXProtocol::PACKET_UBX_SAT, 1);
}

std::map<string /* Talker Id */, std::map<int, SatelliteInfo>> current_sats;

bool sat_found_in_list(SatelliteInfo sat, vector<SatelliteInfo> &sats)
{
  for (auto s : sats)
    if (sat.talker_id == s.talker_id && sat.prn == s.prn)
      return true;
  return false;
}

void Header(vector<SatelliteInfo> &sats)
{
  Serial.println();
  string last_id = "";
  for (auto idkvp : current_sats)
    for (auto prnkvp : idkvp.second)
    {
      auto sat = prnkvp.second;
      auto lastsat = std::prev(idkvp.second.end())->second;
      bool islastelt = sat.talker_id == lastsat.talker_id && sat.prn == lastsat.prn;
      Serial.printf("%-3s%c", 
        sat.talker_id == last_id ? "---" : sat.talker_id.c_str(),
        islastelt ? ' ' : '-');
      last_id = sat.talker_id;
    }
  
  Serial.println(); Serial.flush();

  for (auto idkvp : current_sats)
    for (auto prnkvp : idkvp.second)
    {
      bool found = sat_found_in_list(prnkvp.second, sats);
      if (prnkvp.second.used)
        Serial.printf("\033[32m"); // green
      else if (found)
        Serial.printf("\033[34m"); // blue

      Serial.printf("%03d ", prnkvp.second.prn);

      if (found || prnkvp.second.used)
        Serial.printf("\033[0m"); // back to normal
    }
  Serial.println(); Serial.flush();

  for (auto idkvp : current_sats)
    for (auto prnkvp : idkvp.second)
    {
        auto sat = prnkvp.second;
        auto lastsat = std::prev(idkvp.second.end())->second;
        bool islastelt = sat.talker_id == lastsat.talker_id && sat.prn == lastsat.prn;
        Serial.printf("---%c", islastelt ? ' ' : '-');
    }

  Serial.println(); Serial.flush();
}

#define NO_ELEVATION (-127)
void loop()
{
  static int linecount = 0;
  if (gps.NewSatellitesAvailable())
  {
    auto & sats = gps.GetSatellites();

    // Anything changed?
    vector<SatelliteInfo> newsats;
    vector<SatelliteInfo> changedsats;

    // Step 1: See if any of the sats in unknown
    for (auto sat : sats)
    {
      if (current_sats.find(sat.talker_id) == current_sats.end() ||
        current_sats[sat.talker_id].find(sat.prn) == current_sats[sat.talker_id].end())
      {
        Serial.printf("New: %s.%d\r\n", sat.talker_id.c_str(), sat.prn);
        newsats.push_back(sat);
      }
      else if (current_sats[sat.talker_id][sat.prn].elevation != sat.elevation)
      {
        current_sats[sat.talker_id][sat.prn].elevation = sat.elevation;
        changedsats.push_back(sat);
      }
    }
    for (auto & idkvp : current_sats)
      for (auto & prnkvp : idkvp.second)
      {
        bool found = false;
        for (auto sat : sats)
        {
          if (sat.talker_id == idkvp.first && sat.prn == prnkvp.first)
          {
            found = true;
            break;
          }
        }
        if (!found)
        {
          if (prnkvp.second.elevation != NO_ELEVATION)
          {
            // Serial.printf("Gone: %s.%d\r\n", idkvp.first.c_str(), prnkvp.first);
            changedsats.push_back(prnkvp.second);
            prnkvp.second.elevation = NO_ELEVATION;
          }
        }
      }

    if (!newsats.empty() || !changedsats.empty())
    {
      linecount++;
      if (linecount == 20 || !newsats.empty())
      {
        current_sats.clear();
        linecount = 0;
        for (auto sat : sats)
          current_sats[sat.talker_id][sat.prn] = sat;
        Header(newsats);
      }
      for (auto idkvp : current_sats)
        for (auto prnkvp : idkvp.second)
          if (prnkvp.second.elevation == NO_ELEVATION)
            Serial.printf("    ");
          else
          {
            bool found = sat_found_in_list(prnkvp.second, changedsats) || sat_found_in_list(prnkvp.second, newsats);
            if (found)
              Serial.printf("\033[34m"); // blue
            if (prnkvp.second.elevation < -90 || prnkvp.second.elevation > 90)
              Serial.printf("UNK ");
            else
              Serial.printf("%2d%c ", prnkvp.second.elevation, prnkvp.second.used ? '*' : ' ');
            if (found /* || prnkvp.second.used */)
              Serial.printf("\033[0m"); // back to default
          }
      auto &snap = gps.GetSnapshot();
      if (snap.FixStatus.IsPositionValid())
      {
        Serial.printf("- ");
        Serial.printf("\033[32m"); // green
        Serial.printf("Fix");
        Serial.printf("\033[0m"); // back to default
      }
      else
      {
        Serial.printf("- No Fix");
      }
      Serial.println();
    }
  }
}