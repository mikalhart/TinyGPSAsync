#include <TinyGPSAsync.h>
#if __has_include("../examples/Device.h")
    #include "../examples/Device.h"
#endif

TinyGPSAsync gps;

int totalTime, totalDate;
int failcount = 0;
int loopcount = 0;
constexpr int Fail = 20 * 60; // No more than 20 minutes to get fix
constexpr int TickSize = Fail / 100;
void setup()
{
    Serial.begin(115200);
    delay(3000);

    Serial1.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.printf("Testing to see the average speed to get time and date\r\n");
    Serial.printf("Fail if it takes more than %d minutes from cold start\r\n\r\n", Fail / 60);

    Serial.printf("%-3s  %-5s %-5s\r\n", "Tst", "Time", "Avg");
    Serial.printf("---------------\r\n");
    gps.begin(Serial1);

    UBXProtocol ubx(Serial1);

    // Enable PVR and SAT packets
    ubx.setpacketrate(UBXProtocol::PACKET_UBX_PVT, 1);
    ubx.setpacketrate(UBXProtocol::PACKET_UBX_SAT, 1);

    ubx.setHZ(1000);
}

void loop()
{
    UBXProtocol ubx(Serial1);
    ubx.reset(UBXProtocol::RESET_COLD);

    unsigned long start = millis();
    int elapsed_secs = 0;
    int tt = Fail, td = Fail;
    int avgDateSecs = loopcount == 0 ? 0 : totalDate / loopcount;
    while ((td == Fail || tt == Fail) && elapsed_secs < Fail)
    {
        unsigned long m = millis();
        if ((m - start) / 1000 != elapsed_secs)
        {
            const Snapshot &ss = gps.GetSnapshot();
            elapsed_secs = (m - start) / 1000;
            int m = elapsed_secs / 60;
            int s = elapsed_secs % 60;

            auto t = ss.Time;
            auto d = ss.Date;
            if (tt == Fail && !t.IsVoid())
                tt = elapsed_secs;

            if (tt != Fail && t.IsVoid())
                tt = Fail;

            if (td == Fail && !d.IsVoid())
                td = elapsed_secs;

            if (td != Fail && d.IsVoid())
                td = Fail;

            int avgDateSecs = (totalDate + elapsed_secs) / (loopcount + 1);
            bool foundTime = tt == elapsed_secs && tt != Fail;
            bool foundDate = td == elapsed_secs && td != Fail;
            bool green = USE_ANSI_COLOURING && foundTime && !foundDate;
            bool red = USE_ANSI_COLOURING && failcount > 0;
            Serial.printf("%3d %s%2d:%02d%s %s%2d:%02d%s ", loopcount + 1, 
                green ? "\033[32m" : "", m, s, green ? "\033[0m" : "",
                red ? "\033[31m" : "", avgDateSecs / 60, avgDateSecs % 60, red ? "\033[0m" : "");
            for (int i=0; i<=elapsed_secs; i+=TickSize)
            {
                if (tt != Fail && i < tt && i + TickSize >= tt)
                    Serial.printf("T");
                if (td != Fail && i < td && i + TickSize >= td)
                    Serial.printf("D");
                Serial.printf(i + TickSize > elapsed_secs ? "*" : " ");
            }
                if (gps.DiagnosticCode() != 0)
                    Serial.printf(" %s", gps.DiagnosticString());
            if (elapsed_secs + 1 == Fail)
                Serial.printf("%s FAIL%s", USE_ANSI_COLOURING ? "\033[31m" : "", USE_ANSI_COLOURING ? "\033[0m" : "");
            Serial.printf("\r");
        }
    }
    if (elapsed_secs >= Fail)
        ++failcount;
    totalDate += td;
    totalTime += tt;
    ++loopcount;
    avgDateSecs = totalDate / loopcount;
    Serial.println();
}
