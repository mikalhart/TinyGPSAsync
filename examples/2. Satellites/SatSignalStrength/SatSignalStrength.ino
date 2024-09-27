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

  Serial.println("SatSignalStrength.ino");
  Serial.println("Examining reported satellite signal strength");
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

  // Draw the first line: talker ids and dashes
  for (auto idkvp : current_sats)
    for (auto prnkvp : idkvp.second)
    {
      auto sat = prnkvp.second;
      SatelliteInfo lastsat = std::prev(std::prev(current_sats.end())->second.end())->second;
      bool islastelt = sat.talker_id == lastsat.talker_id && sat.prn == lastsat.prn;
      Serial.printf("%-2s-%c", 
        sat.talker_id == last_id ? "--" : sat.talker_id.c_str(),
        islastelt ? ' ' : '-');
      last_id = sat.talker_id;
    }
  
  Serial.println(); Serial.flush();

  // Draw the second line: satellite PRNs
  for (auto idkvp : current_sats)
    for (auto prnkvp : idkvp.second)
    {
      bool found = sat_found_in_list(prnkvp.second, sats);
      if (found && USE_ANSI_COLOURING)
        Serial.printf(prnkvp.second.used ? "\033[32m" /* green */: "\033[34m" /* blue */);
      Serial.printf("%03d ", prnkvp.second.prn);
      if (found && USE_ANSI_COLOURING)
        Serial.printf("\033[0m"); // back to normal
    }
  Serial.println(); Serial.flush();

  // Draw the third line: mostly dashes
  for (auto idkvp : current_sats)
    for (auto prnkvp : idkvp.second)
    {
        auto sat = prnkvp.second;
        SatelliteInfo lastsat = std::prev(std::prev(current_sats.end())->second.end())->second;
        bool islastelt = sat.talker_id == lastsat.talker_id && sat.prn == lastsat.prn;
        Serial.printf("---%c", islastelt ? ' ' : '-');
    }

  Serial.println(); Serial.flush();
}

#define NO_SNR (-127)
void loop()
{
  static int linecount = 0;
  if (gps.NewSatellitesAvailable())
  {
    auto & sat_report = gps.GetSatellites();

    // Anything changed?
    vector<SatelliteInfo> newsats;
    vector<SatelliteInfo> changedsats;

    // Step 1: See if any of the reported sats is new or changed
    for (auto sat : sat_report)
    {
      if (current_sats.find(sat.talker_id) == current_sats.end() ||
        current_sats[sat.talker_id].find(sat.prn) == current_sats[sat.talker_id].end())
      {
        // Serial.printf("\r\n\nNew: %s.%03d\r\n", sat.talker_id.c_str(), sat.prn);
        newsats.push_back(sat);
      }
      else if (current_sats[sat.talker_id][sat.prn].snr != sat.snr)
      {
        current_sats[sat.talker_id][sat.prn].snr = sat.snr;
        changedsats.push_back(sat);
      }
    }

    // Check all the currently know sats to see
    for (auto & idkvp : current_sats)
      for (auto & prnkvp : idkvp.second)
      {
        bool found = false;
        for (auto sat : sat_report)
        {
          if (sat.talker_id == idkvp.first && sat.prn == prnkvp.first)
          {
            found = true;
            break;
          }
        }

        if (!found && prnkvp.second.snr != NO_SNR)
        {
          changedsats.push_back(prnkvp.second);
          prnkvp.second.snr = NO_SNR;
        }
      }

    // If anything has changed, draw it
    if (!newsats.empty() || !changedsats.empty())
    {
      linecount++;

      // Draw a new header, as appropriate
      if (linecount == 20 || !newsats.empty())
      {
        current_sats.clear();
        linecount = 0;
        for (auto sat : sat_report)
          current_sats[sat.talker_id][sat.prn] = sat;
        Serial.println();
        Header(newsats);
      }

      for (auto idkvp : current_sats)
        for (auto prnkvp : idkvp.second)
          if (prnkvp.second.snr == NO_SNR)
            Serial.printf("    ");
          else
          {
            bool found = sat_found_in_list(prnkvp.second, changedsats) || sat_found_in_list(prnkvp.second, newsats);
            if (found && USE_ANSI_COLOURING)
              Serial.printf("\033[34m"); // blue
            Serial.printf("%2d%c ", prnkvp.second.snr, prnkvp.second.used ? '*' : ' ');
            if (found && USE_ANSI_COLOURING)
              Serial.printf("\033[0m"); // back to default
          }
      auto &snap = gps.GetSnapshot();
      if (snap.FixStatus.IsPositionValid())
      {
        Serial.printf("- ");
        if (USE_ANSI_COLOURING) Serial.printf("\033[32m"); // green
        Serial.printf("Fix   ");
        if (USE_ANSI_COLOURING) Serial.printf("\033[0m"); // back to default
      }
      else
      {
        Serial.printf("- No Fix");
      }
      Serial.printf("\r");
    }
  }
}