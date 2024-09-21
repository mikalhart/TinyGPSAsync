#include <TinyGPSAsync.h>
#if __has_include("../examples/Device.h")
    #include "../examples/Device.h"
#endif

TinyGPSAsync gps;

void setup()
{
  Serial.begin(115200);
  delay(3000);

#if defined(LED_BUILTIN)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
#endif

  Serial1.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}

int totalTime, totalDate, gtl, gtf;
int loopcount = 0;

void loop()
{
    // Cold reset
    Serial.println();
    Serial.println("*** Cold reset ***                                                                                          ");
    Doit();

    ++loopcount;
    Serial.printf("\n");
    Serial.printf("*** Averages in %d run%s ***\n", loopcount, loopcount == 1 ? "" : "s");
    Serial.printf("Time Date  Loc Fix\n");
    Serial.printf(" %3d  %3d  %3d %3d\n",
        totalTime / loopcount, totalDate / loopcount, gtl / loopcount, gtf / loopcount);
}

void Doit()
{
    static TinyGPSAsync gps;
    gps.begin(Serial1);

    UBXProtocol ubx(Serial1);
    ubx.reset(UBXProtocol::RESET_COLD);

    // Enable PVR and SAT packets
    ubx.setpacketrate(UBXProtocol::PACKET_NMEA_GGA, 0);
    ubx.setpacketrate(UBXProtocol::PACKET_NMEA_GSV, 0);
    ubx.setpacketrate(UBXProtocol::PACKET_NMEA_GSA, 0);
    ubx.setpacketrate(UBXProtocol::PACKET_NMEA_VTG, 0);
    ubx.setpacketrate(UBXProtocol::PACKET_NMEA_RMC, 0);
    ubx.setpacketrate(UBXProtocol::PACKET_NMEA_GLL, 0);
    ubx.setpacketrate(UBXProtocol::PACKET_UBX_PVT, 1);
    ubx.setpacketrate(UBXProtocol::PACKET_UBX_SAT, 1);
    
    ubx.setHZ(1000);
    for (unsigned long start = millis(); millis() - start < 3000;)
    {
        if (gps.NewSentenceAvailable())
        {
            auto &p = gps.GetLatestPacket();
            if (p.IsNMEA())
            {
                auto &sent = p.As<NmeaPacket>();
                if (sent.SentenceId() == "TXT")
                    Serial.printf("%03d: %s                                                                  \n", (millis() - start) / 1000, sent.ToString().c_str());
            }
            else if (p.IsUBX())
            {
                auto &pack = p.As<UbxPacket>();
                if (pack.Id() != 0x7 && pack.Id() != 0x35)
                    Serial.printf("%03d: %s                                                                  \n", (millis() - start) / 1000, pack.ToString().c_str());
            }
        }
    }

    int startsec = millis() / 1000;
    int seconds = -1;
    int max = 16 * 60; // No more than 16 minutes to get fix
    int tt = max, td = max, tl = max, tf = max;
    while ((tl == max || td == max || tt == max || tf == max) && seconds - startsec < max)
    {
        unsigned long m = millis();
        if (m / 1000 != seconds)
        {
            const Snapshot &ss = gps.GetSnapshot();
            seconds = m / 1000;
            auto t = ss.Time;
            auto d = ss.Date;
            auto l = ss.Location;
            auto & f = ss.FixStatus;
            auto & stats = gps.GetStatistics();
            auto & sats = gps.GetSatellites();
            int usedSatCount = 0;
            for (auto sat : sats)
                if (sat.used)
                    ++usedSatCount;
            Serial.printf("%d: %04d: Seen=%d Used=%d %c%c%c%c Diags=%s\r",
                loopcount + 1, seconds - startsec, sats.size(), usedSatCount,
                t.IsVoid() ? '.' : 'T', d.IsVoid() ? '.' : 'D', l.IsVoid() ? '.': 'L', f.IsVoid() ? '.' : f.Value(),
                gps.DiagnosticString());
            if (tt == max && !t.IsVoid())
            {
                Serial.printf("%03d: Got Time (%02d:%02d:%02d)                                                                                      \n", seconds - startsec, t.Hour(), t.Minute(), t.Second());
                tt = seconds - startsec;
            }

            if (tt != max && t.IsVoid())
            {
                Serial.printf("%03d: Lost Time (%02d:%02d:%02d)                                                                                      \n", seconds - startsec, t.Hour(), t.Minute(), t.Second());
                tt = max;
            }

            if (td == max && !d.IsVoid())
            {
                Serial.printf("%03d: Got Date (%02d-%02d-%04d)                                                                                      \n", seconds - startsec, d.Day(), d.Month(), d.Year());
                td = seconds - startsec;
            }

            if (td != max && d.IsVoid())
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

    totalDate += td;
    gtf += tf;
    gtl += tl;
    totalTime += tt;

    gps.end();
}