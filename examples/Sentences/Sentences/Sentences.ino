#include <Arduino.h>
#include <TinyGPSAsync.h>
#include <set>

TinyGPSAsync gps;

/* Here's where you customize for your personal ESP32 setup */
#define GPS_RX_PIN D1
#define GPS_TX_PIN D0
#define GPS_BAUD 115200
#define GPSSerial Serial1

void setup()
{
  Serial.begin(115200);
  GPSSerial.setRxBufferSize(20000);
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
 
  delay(2000); // Allow ESP32 serial to initialize

  Serial.println("Sentences.ino");
  Serial.println("Looking at raw NMEA sentences");
  Serial.println();

  gps.begin(GPSSerial);
}

// This code displays all the Sentence Ids that have been received,
// and then the latest GGA sentence
void loop()
{
#if false
  if (gps.NewUbxPacketAvailable())
  {
    static int countPvt = 0;
    auto & packet = gps.GetUbxPackets().LastUbxPacket.Packet();
//    Serial.printf("UBX Sync:[%02X,%02X] Class: %02X Id: %02X Chksum:[%02X,%02X] Payload: ", 
//      packet.sync[0], packet.sync[1], packet.clss, packet.id, packet.chksum[0], packet.chksum[1]);
    for (int i=0; i<10 && i<packet.payload.size(); ++i)
      Serial.printf("%02X ", packet.payload[i]);
    if (packet.clss == 1 && packet.id == 7)
    {
      uint32_t ns = packet.payload[16] + 256 * packet.payload[17] + 256 * 256 * packet.payload[18] + 256 * 256 * 256 * packet.payload[19];
      uint16_t year = packet.payload[4] + 256 * packet.payload[5];
      uint8_t month = packet.payload[6];
      uint8_t day = packet.payload[7];
      uint8_t hour = packet.payload[8];
      uint8_t minute = packet.payload[9];
      uint8_t second = packet.payload[10];
      uint8_t sats = packet.payload[23];
      double lat = *(int32_t *)&packet.payload[28] / 1e7;
      double lng = *(int32_t *)&packet.payload[24] / 1e7;
      ++countPvt;
      Serial.printf("%02d:%02d:%02d.%09d %02d-%02d-%04d %2.8f %2.8f sats=%d Hz=%.1f    \r", hour, minute, second, ns, day, month, year, lat, lng, sats, 1000.0 * count17 / millis());
    }
    auto & packets = gps.GetUbxPackets().AllUbxPackets;
//    for (auto kvp : packets)
//      Serial.print(string("PACKET!) " + kvp.second.String() + "   ").c_str());
//    Serial.println();
  }
#endif
#if true
  
  static set<string> talkers;
  if (gps.NewSentenceAvailable())
  {
    auto & sentences = gps.GetSentences();
    Serial.print("Sentence IDs seen: ");
    for (auto s:sentences.AllSentences)
        Serial.printf("%s ", s.second.SentenceId().c_str());
    Serial.print("    Talker IDs seen: ");
    for (auto s:sentences.AllSentences)
      talkers.insert(s.second.TalkerId());
    for (auto talker:talkers)
        Serial.printf("%s ", talker.c_str());
    
    auto & snapshot = gps.GetSnapshot();
    Serial.printf("    SATS = %d                  \r", snapshot.SatelliteCount.Value());
    
//    delay(1000);
//    auto gga = sentences["GGA"].String();
//    Serial.printf("      Latest GGA sentence: %s\r", gga.c_str());
  }
#endif
}
