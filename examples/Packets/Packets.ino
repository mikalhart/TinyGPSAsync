#include <Arduino.h>
#include <TinyGPSAsync.h>
#include <map>

#if false // Quescan M10FD    
#define GPSNAME "Quescan M10FD"
#define RX D1
#define TX D0
#define GPSBAUD 38400
#define FACTORYRESETBYTES { 0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x02, 0x1A, 0x0A }
#define COLDRESETBYTES {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0xFF, 0xFF, 0x02, 0x00, 0x0E, 0x61}
#define WARMRESETBYTES {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x11, 0x6C}
#define HOTRESETBYTES  {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x10, 0x68}
#define INITIALSTART { 0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x04, 0x01, 0xFE, 0x10 }
#define isValidDateTime(d, t) true
#elif true // Sparkfun SAM-M10Q
#define GPSNAME "Sparkfun SAM-M10Q"
#define RX D1
#define TX D0
#define GPSBAUD 9600
#define FACTORYRESETBYTES { 0xB5, 0x62, 0x06, 0x09, 0x0D, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x02, 0x1A, 0x0A }
#define COLDRESETBYTES {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0xFF, 0xFF, 0x02, 0x00, 0x0E, 0x61}
#define WARMRESETBYTES {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x11, 0x6C}
#define HOTRESETBYTES  {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x10, 0x68}
#define INITIALSTART { 0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x04, 0x01, 0xFE, 0x10 }
#define isValidDateTime(d, t) true
#endif

 // For 5 seconds, wait for initial messages
 TinyGPSAsync gps;

static void updateChecksum(byte &ck_A, byte &ck_B, byte b)
{
    ck_A += b;
    ck_B += ck_A;
}

void sendpacket(Stream &stream, uint8_t clss, uint8_t id, vector<uint8_t> &payload)
{
  byte ck_A = 0, ck_B = 0;

  // calculate checksum
  updateChecksum(ck_A, ck_B, clss);
  updateChecksum(ck_A, ck_B, id);
  updateChecksum(ck_A, ck_B, (byte)(payload.size() & 0xFF));
  updateChecksum(ck_A, ck_B, (byte)((payload.size() >> 8) & 0xFF));
  for (int i=0; i<payload.size(); ++i)
      updateChecksum(ck_A, ck_B, payload[i]);

  // write packet
  stream.write(0xB5); stream.write(0x62);
  stream.write(clss); stream.write(id);
  stream.write(payload.size() & 0xFF);
  stream.write((payload.size() >> 8) & 0xFF);
  for (auto ch: payload)
    stream.write(ch);

  stream.write(ck_A);
  stream.write(ck_B);
}

#define PACKET_NMEA_GGA 0x209100BB
#define PACKET_NMEA_GSV 0x209100C5
#define PACKET_NMEA_GSA 0x209100C0
#define PACKET_NMEA_VTG 0x209100B1
#define PACKET_NMEA_RMC 0x209100AC
#define PACKET_NMEA_GLL 0x209100CA
#define PACKET_UBX_PVT  0x20910007

void setpacketrate(Stream &stream, uint32_t packet_type, uint8_t rate)
{
    vector<uint8_t> payload = {0x01, 0x01, 0x00, 0x00};
    payload.reserve(9);
    payload.push_back((uint8_t)((packet_type >> 0) & 0xFF));
    payload.push_back((uint8_t)((packet_type >> 8) & 0xFF));
    payload.push_back((uint8_t)((packet_type >> 16) & 0xFF));
    payload.push_back((uint8_t)((packet_type >> 24) & 0xFF));
    payload.push_back(rate);
    sendpacket(stream, 0x06, 0x8A, payload);
    delay(100);
}

void setHZ(Stream &stream, uint16_t ms)
{
    uint32_t cfg_rate_meas = 0x30210001;
    uint32_t cfg_rate_nav  = 0x30210002;
    vector<uint8_t> payload = {0x01, 0x01, 0x00, 0x00};
    payload.reserve(10);
    payload.push_back((uint8_t)((cfg_rate_meas >> 0) & 0xFF));
    payload.push_back((uint8_t)((cfg_rate_meas >> 8) & 0xFF));
    payload.push_back((uint8_t)((cfg_rate_meas >> 16) & 0xFF));
    payload.push_back((uint8_t)((cfg_rate_meas >> 24) & 0xFF));
    payload.push_back((uint8_t)((ms >> 0) & 0xFF));
    payload.push_back((uint8_t)((ms >> 8) & 0xFF));
    sendpacket(stream, 0x06, 0x8A, payload);

    payload = {0x01, 0x01, 0x00, 0x00};
    payload.push_back((uint8_t)((cfg_rate_nav >> 0) & 0xFF));
    payload.push_back((uint8_t)((cfg_rate_nav >> 8) & 0xFF));
    payload.push_back((uint8_t)((cfg_rate_nav >> 16) & 0xFF));
    payload.push_back((uint8_t)((cfg_rate_nav >> 24) & 0xFF));
    payload.push_back((uint8_t)((1 >> 0) & 0xFF));
    payload.push_back((uint8_t)((1 >> 8) & 0xFF));
    sendpacket(stream, 0x06, 0x8A, payload);
}

void setup()
{
  pinMode(D7, OUTPUT);
  digitalWrite(D7, HIGH);

  Serial.begin(115200);
  delay(3000);

  // Serial1.setRxBufferSize(2048);
  Serial1.begin(GPSBAUD, SERIAL_8N1, RX, TX);

  for (auto ch: FACTORYRESETBYTES)
    Serial1.write(ch);
  delay(500);

  setpacketrate(Serial1, PACKET_NMEA_GGA, 1);
  setpacketrate(Serial1, PACKET_NMEA_GSV, 0);
  setpacketrate(Serial1, PACKET_NMEA_GSA, 0);
  setpacketrate(Serial1, PACKET_NMEA_VTG, 0);
  setpacketrate(Serial1, PACKET_NMEA_RMC, 0);
  setpacketrate(Serial1, PACKET_NMEA_GLL, 0);
  setpacketrate(Serial1, PACKET_UBX_PVT, 0);
  
  setHZ(Serial1, 100);

  gps.begin(Serial1);
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
    // if (lines < 2) lines = 2;

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
    if (gps.NewUnknownPacketAvailable())
    {
        auto &packet = gps.GetUnknownPacket();
        render(packet, "UNKNOWN");
    }
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
}