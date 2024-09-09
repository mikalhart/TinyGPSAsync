#include <Arduino.h>
#include <TinyGPSAsync.h>
#include <map>

#if false // NEO-6M
 #define GPSBAUD 9600
#define COLDRESETBYTES {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0xFF, 0xFF, 0x02, 0x00, 0x0E, 0x61}
#define WARMRESETBYTES {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x11, 0x6C}
#define HOTRESETBYTES  {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x10, 0x68}
#define isValidDateTime(d, t) true
#elif false // L80-R
#define RX D0
#define TX D1
#define GPSBAUD 9600
// #define COLDRESETBYTES "$PMTK103*30\r\n" // second coldest
#define GPSNAME "L80-R"
#define COLDRESETBYTES "$PMTK104*37\r\n"
#define WARMRESETBYTES "$PMTK102*31\r\n"
#define HOTRESETBYTES  "$PMTK101*32\r\n"
#define isValidDateTime(d, t) (d.Year() != 2080 || d.Month() != 1)
#elif true // Quescan M10FD    
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
#elif false // Small Chinese clone
#define GPSNAME "Beitian AT6558F"
#define RX D0
#define TX D1
#define GPSBAUD 9600
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
#define COLDRESETBYTES {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0xFF, 0xFF, 0x02, 0x00, 0x0E, 0x61}
#define WARMRESETBYTES {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x01, 0x00, 0x02, 0x00, 0x11, 0x6C}
#define HOTRESETBYTES  {0xB5, 0x62, 0x06, 0x04, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00, 0x10, 0x68}
#define INITIALSTART { 0xB5, 0x62, 0x06, 0x01, 0x03, 0x00, 0xF0, 0x04, 0x01, 0xFE, 0x10 }
#define isValidDateTime(d, t) true
#endif

void setup()
{
  pinMode(D7, OUTPUT);
  digitalWrite(D7, LOW); // GPS off for now

  Serial.begin(115200);
//  delay(3000);

#if defined(LED_BUILTIN)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
#endif

  Serial1.setRxBufferSize(20000);
  Serial1.begin(GPSBAUD, SERIAL_8N1, RX, TX);

#if false
while (true)
{
    if (Serial1.available())
        Serial.write(Serial1.read());
    if (Serial.available())
        Serial1.write(Serial.read());
}
#endif

#if defined(INITIALSTART)
{
    for (auto ch: INITIALSTART)
        Serial1.write(ch);
}
#endif

#if false
  for (unsigned long s = millis(); millis() - s < 10000;)
  {
    if (Serial1.available())
        Serial.write(Serial1.read());
  }
#endif
    digitalWrite(D7, HIGH);

    // For 5 seconds, wait for initial messages
    TinyGPSAsync gps;
    gps.begin(Serial1);

    while (true)
    {
        if (gps.NewUnknownPacketAvailable())
        {
            auto &packet = gps.GetUnknownPacket();
            Serial.printf("UNKNOWN %d - ", packet.size());
            for (int i=0; i<(packet.size() < 20 ? packet.size() : 20); ++i)
                Serial.printf("%02X(%c) ", packet[i], isprint(packet[i]) ? packet[i] : '.');
            Serial.println();
        }
        if (gps.NewSentenceAvailable())
        {
            auto &sent = gps.GetSentences();
            Serial.printf("    SENT: %s\n", sent.LastSentence.ToString().c_str());
        }
        if (gps.NewUbxPacketAvailable())
        {
            auto &ubx = gps.GetUbxPackets();
            Serial.printf("        UBX: %d.%d %s\n", ubx.LastUbxPacket.Class(), ubx.LastUbxPacket.Id(), ubx.LastUbxPacket.ToString().c_str());
        }
    }
#if false
    std::map<string, int> mymap;
    TinyGPSAsync gps;
    gps.begin(Serial1);
    unsigned long last = 0;
    while (true)
    {
        static bool first = true;
        if (gps.NewSentenceAvailable())
        {
            auto &sent = gps.GetSentences().LastSentence;
            if (first)
            {
                Serial.printf("First: '%s'\n", sent.String().c_str());
                first = false;
            }

            mymap[sent.SentenceId()]++;
        }
        unsigned long m = millis();
        if (last / 10000 != m / 10000)
        {
            Serial.printf("%d: ", uint32_t(last / 10000));
            for (auto kvp:mymap)
                Serial.printf("%s: %d  ", kvp.first.c_str(), kvp.second);
            auto &stats = gps.GetStatistics();
            Serial.printf("Official RMC: %d  Official GGA: %d\n", stats.rmcCount, stats.ggaCount);
            last = m;
            mymap.clear();
        }
    }
#endif
}

struct 
{
    int tt, td, tl, tf, ts[4];
} cold, warm, hot;
int loopcount = 0;

void loop()
{
    digitalWrite(D7, HIGH);

    // For 5 seconds, wait for initial messages
    TinyGPSAsync gps;
    gps.begin(Serial1);
    Serial.println();
    Serial.printf("*** Startup %s ***                                                                                          \n", GPSNAME);
#if false
    for (unsigned long start = millis(); millis() - start < 5000;)
    {
        if (gps.NewSentenceAvailable())
        {
            auto &s = gps.GetSentences();
            if (s.LastSentence.SentenceId() == "TXT")
                Serial.printf("%03d: %s                                                                  \n", (millis() - start) / 1000, s.LastSentence.String().c_str());
        }
    }
#endif
    gps.end();

    // Cold reset
    Serial.println();
    Serial.println("*** Cold reset ***                                                                                          ");
    Doit(1, COLDRESETBYTES);

    Serial.println();
    Serial.println("*** Warm reset ***                                                                                           ");
    Doit(2, WARMRESETBYTES);

    Serial.println();
    Serial.println("*** Hot reset ***                                                                                           ");
    Doit(3, HOTRESETBYTES);

    // Turn off GPS
    digitalWrite(D7, LOW);
//    delay(1000);

    ++loopcount;
    Serial.printf("\n");
    Serial.printf("*** Averages in %d run%s ***\n", loopcount, loopcount == 1 ? "" : "s");
    Serial.printf("Cold -----------------------   Warm -----------------------   Hot ------------------------\n");
    Serial.printf("Time Date Sat3 Sat4  Loc Fix   Time Date Sat3 Sat4  Loc Fix   Time Date Sat3 Sat4  Loc Fix\n");
    Serial.printf(" %3d  %3d  %3d  %3d  %3d %3d    %3d  %3d  %3d  %3d  %3d %3d    %3d  %3d  %3d  %3d  %3d %3d\n",
        cold.tt / loopcount, cold.td / loopcount, cold.ts[2] / loopcount, cold.ts[3] / loopcount, cold.tl / loopcount, cold.tf / loopcount,
        warm.tt / loopcount, warm.td / loopcount, warm.ts[2] / loopcount, warm.ts[3] / loopcount, warm.tl / loopcount, warm.tf / loopcount,
        hot.tt / loopcount, hot.td / loopcount, hot.ts[2] / loopcount, hot.ts[3] / loopcount, hot.tl / loopcount, hot.tf / loopcount
        );
}

void Doit(int count, std::vector<byte> cmd)
{
    TinyGPSAsync gps;
    gps.begin(Serial1);
    for (byte c: cmd)
      Serial1.write(c);

    for (unsigned long start = millis(); millis() - start < 3000;)
    {
        if (gps.NewSentenceAvailable())
        {
            auto &s = gps.GetSentences();
            if (s.LastSentence.SentenceId() == "TXT")
                Serial.printf("%03d: %s                                                                  \n", (millis() - start) / 1000, s.LastSentence.ToString().c_str());
        }
        if (gps.NewUbxPacketAvailable())
        {
            auto &p = gps.GetUbxPackets();
            if (p.LastUbxPacket.Id() != 0x7 && p.LastUbxPacket.Id() != 0x35)
                Serial.printf("%03d: %s                                                                  \n", (millis() - start) / 1000, p.LastUbxPacket.ToString().c_str());
        }
    }


    int startsec = millis() / 1000;
    int seconds = -1;
    int max = count == 1 ? 3000 : 300;
    int tt = max, td = max, tl = max, tf = max, ts[4] = {max, max, max, max};
    while ((tl == max || ts[3] == max || td == max || tt == max || tf == max) && seconds - startsec < max)
    {
        unsigned long m = millis();
        if (m / 1000 != seconds)
        {
            const Snapshot &ss = gps.GetSnapshot();
            seconds = m / 1000;
            auto t = ss.Time;
            auto d = ss.Date;
            auto l = ss.Location;
            auto s = ss.SatelliteCount;
            auto & f = ss.FixStatus;
            auto & stats = gps.GetStatistics();
            char timestring[] = "  :  :  ";
            char datestring[] = "  -  -    ";
            char latstring[] = "[   lat   ]";
            char lngstring[] = "[   lng   ]";
            if (!t.IsVoid())
                sprintf(timestring, "%02d:%02d:%02d", t.Hour(), t.Minute(), t.Second());
            if (!d.IsVoid())
                sprintf(datestring, "%02d-%02d-%04d", d.Day(), d.Month(), d.Year());
            if (!l.IsVoid())
            {
                sprintf(latstring, "% 2.6f", l.Lat());
                sprintf(lngstring, "% 3.6f", l.Lng());
            }
            Serial.printf("%03d: %s %s %s %s Fix=%c Sats=%d GGA=%d RMC=%d UBX.Pvt=%d Diags=%s\r", seconds - startsec,
                datestring, timestring, latstring, lngstring, f.Value(), s.Value(),
                stats.ggaCount, stats.rmcCount, stats.ubxNavPvtCount, gps.DiagnosticString());
            if (tt == max && !t.IsVoid() && isValidDateTime(d, t))
            {
                Serial.printf("%03d: Got Time (%02d:%02d:%02d)                                                                                      \n", seconds - startsec, t.Hour(), t.Minute(), t.Second());
                tt = seconds - startsec;
            }

            if (tt != max && (t.IsVoid() || !isValidDateTime(d, t)))
            {
                Serial.printf("%03d: Lost Time (%02d:%02d:%02d)                                                                                      \n", seconds - startsec, t.Hour(), t.Minute(), t.Second());
                tt = max;
            }

            if (td == max && !d.IsVoid() && isValidDateTime(d, t))
            {
                Serial.printf("%03d: Got Date (%02d-%02d-%04d)                                                                                      \n", seconds - startsec, d.Day(), d.Month(), d.Year());
                td = seconds - startsec;
            }

            if (td != max && d.Age() > 2000)
            {
                Serial.printf("%03d: Lost Date (%02d-%02d-%04d)                                              \n", seconds - startsec, d.Day(), d.Month(), d.Year());
                td = max;
            }
            if (tl == max && !l.IsVoid())
            {
                Serial.printf("%03d: Got Location (%.6f %.6f)                                                                                       \n", seconds - startsec, l.Lat(), l.Lng());
                tl = seconds - startsec;
            }

            if (tl != max && l.IsVoid())
            {
                Serial.printf("%03d: Lost Location (%.6f %.6f)                                                                                       \n", seconds - startsec, l.Lat(), l.Lng());
                tl = max;
            }

            if (tf == max && f.IsPositionValid())
            {
                Serial.printf("%03d: Valid Fix established                                                                                       \n", seconds - startsec);
                tf = seconds - startsec;
            }

            if (tf != max && !f.IsPositionValid())
            {
                Serial.printf("%03d: Valid Fix lost                                                                                             \n", seconds - startsec);
                tf = max;
            }
            
            for (int i=2; i<4; ++i)
            {
                if (ts[i] == max && s.Value() > i)
                {
                    Serial.printf("%03d: Got Sat #%d                                                                                               \n", seconds - startsec, i + 1);
                    ts[i] = seconds - startsec;
                }

                if (ts[i] != max && s.Value() <= i)
                {
                    Serial.printf("%03d: Lost Sat #%d                                                                                               \n", seconds - startsec, i + 1);
                    ts[i] = max;
                }
            }
        }
    }
    Serial.println();
    if (tt == max)
        Serial.printf("%03d: Never could get Time                                              \n", max);
    if (td == max)
        Serial.printf("%03d: Never could get Date                                              \n", max);
    if (tl == max)
        Serial.printf("%03d: Never could get Location                                          \n", max);
    if (tf == max)
        Serial.printf("%03d: Never could get Fix                                               \n", max);
    for (int i=2; i<4; ++i)
    {
        if (ts[i] == max)
            Serial.printf("%03d: Never could get Sat #%d                                               \n", max, i + 1);
    }

    auto & stats = count == 1 ? cold : count == 2 ? warm : hot;
    stats.td += td;
    stats.tf += tf;
    stats.tl += tl;
    stats.tt += tt;
    stats.ts[2] += ts[2];
    stats.ts[3] += ts[3];

    gps.end();
}