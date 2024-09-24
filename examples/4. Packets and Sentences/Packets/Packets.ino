#include <TinyGPSAsync.h>
#if __has_include("../examples/Device.h")
    #include "../examples/Device.h"
#endif

#include <map>

TinyGPSAsync gps;

void setup()
{
  pinMode(D7, OUTPUT);
  digitalWrite(D7, HIGH);

  Serial.begin(115200);
  delay(3000);

  // Serial1.setRxBufferSize(2048);
  Serial1.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

#if 0
#define FACTORYRESETBYTES { 0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x02, 0x1A, 0x0A }

  for (auto ch: FACTORYRESETBYTES)
    Serial1.write(ch);
  delay(500);
#endif

  gps.begin(Serial1);

  UBXProtocol ubx(Serial1);
  ubx.setpacketrate(UBXProtocol::PACKET_NMEA_GGA, 0);
  ubx.setpacketrate(UBXProtocol::PACKET_NMEA_GSV, 0);
  ubx.setpacketrate(UBXProtocol::PACKET_NMEA_GSA, 0);
  ubx.setpacketrate(UBXProtocol::PACKET_NMEA_VTG, 0);
  ubx.setpacketrate(UBXProtocol::PACKET_NMEA_RMC, 1);
  ubx.setpacketrate(UBXProtocol::PACKET_NMEA_GLL, 0);
  ubx.setpacketrate(UBXProtocol::PACKET_UBX_PVT, 1);
  ubx.setpacketrate(UBXProtocol::PACKET_UBX_SAT, 1);
  
  ubx.setHZ(1000);
}

const int ITEMSPERROW = 16;
void renderbuf(const vector<uint8_t> &data, int row)
{
    for (int i=0; i<ITEMSPERROW; ++i)
    {
        int offset = ITEMSPERROW * row + i;
        if (offset < data.size())
            Serial.printf("%02X ", data[offset]);
        else
            Serial.printf("   ");
    }

    Serial.print("  ");

    for (int i=0; i<ITEMSPERROW; ++i)
    {
        int offset = ITEMSPERROW * row + i;
        if (offset < data.size())
            Serial.printf("%c", isprint(data[offset]) ? data[offset] : '.');
        else
            Serial.printf(" ");
    }

    Serial.print("   ");
}

void render(const vector<uint8_t> &data, const char *summary)
{
    unsigned long m = millis();
    uint mn = m / 60000;
    uint sc = (m / 1000) % 60;
    int lines = (data.size() + ITEMSPERROW - 1) / ITEMSPERROW;

    auto &snap = gps.GetSnapshot();

    for (int i=0; i<lines; ++i)
    {
        if (i == 0)
        {
            Serial.printf("%02u:%02u - ", mn, sc);
            renderbuf(data, 0);
            Serial.printf("%-16s ", summary);
            if (!snap.Time.IsVoid())
                Serial.printf("%02d:%02d:%02d.%02d ", snap.Time.Hour(), snap.Time.Minute(), snap.Time.Second(), snap.Time.Nanos() / 10000000);
            if (!snap.Date.IsVoid())
                Serial.printf("%02d-%02d-%04d ", snap.Date.Day(), snap.Date.Month(), snap.Date.Year());
            if (!snap.Location.IsVoid())
                Serial.printf("(%2.6f,%2.6f) ", snap.Location.Lat(), snap.Location.Lng());
            if (!snap.Altitude.IsVoid())
                Serial.printf("Alt=%3.2f ", snap.Altitude.Meters());
            if (!snap.Course.IsVoid())
                Serial.printf("Crs=%3.2f ", snap.Course.Degrees());
            if (!snap.Speed.IsVoid())
                Serial.printf("Spd=%3.2f ", snap.Speed.Mps());
            if (!snap.SatelliteCount.IsVoid() && snap.SatelliteCount.Value() > 0)
                Serial.printf("Sats=%d ", snap.SatelliteCount.Value());
        }
        else
        {
            Serial.printf("        ");
            if (mn >= 100)
                Serial.printf(" ");
            renderbuf(data, i);
        }
        Serial.println();
    }
}

void loop()
{
    if (gps.NewPacketAvailable())
    {
        auto &packet = gps.GetLatestPacket();
        char buf[32] = "UNKNOWN";
        if (packet.IsNMEA())
        {
            auto &sentence = packet.As<NmeaPacket>();
            sprintf(buf, "%c %6s", sentence.ChecksumValid() ? 'Y' : 'N', sentence.ToString().substr(0, 6).c_str());
        }
        else if (packet.IsUBX())
        {
            auto &last = packet.As<UbxPacket>();
            const char *pctType = "";
            if (last.Class() == 1)
                pctType = last.Id() == 7 ? "NAV-PVT " : last.Id() == 0x35 ? "NAV-SAT " : last.Id() == 3 ? "NAV-STAT " : "";
            
            sprintf(buf, "%c %s%d.%x", last.ChecksumValid() ? 'Y' : 'N', pctType, last.Class(), last.Id());
        }
        else
        {

        }
        render(packet.ToBuffer(), buf);
    }
#if false
    if (gps.NewSentenceAvailable())
    {
        auto &sentences = gps.GetSentences();
        char buf[32];
        sprintf(buf, "%c %6s", sentences.LastSentence.ChecksumValid() ? 'Y' : 'N', sentences.LastSentence.ToString().substr(0, 6).c_str());
        render(sentences.LastSentence.ToBuffer(), buf);
    }
    if (gps.NewUbxPacketAvailable())
    {
        auto &packets = gps.GetUbxPackets();
        char buf[32];
        auto &last = packets.LastUbxPacket;
        const char *pctType = "";
        if (last.Class() == 1)
            pctType = last.Id() == 7 ? "NAV-PVT " : last.Id() == 0x35 ? "NAV-SAT " : last.Id() == 3 ? "NAV-STAT " : "";
        
        sprintf(buf, "%c %s%d.%x", last.ChecksumValid() ? 'Y' : 'N', pctType, last.Class(), last.Id());
        render(packets.LastUbxPacket.ToBuffer(), buf);
    }
#endif
}