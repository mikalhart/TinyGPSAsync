#include <TinyGPSAsync.h>
#if __has_include("../examples/Device.h")
    #include "../examples/Device.h"
#endif

TinyGPSAsync gps;

void setup()
{
  Serial.begin(115200);
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  UBXProtocol ubx(GPSSerial);

  delay(2000); // Allow ESP32 serial to initialize

  ubx.setpacketrate(UBXProtocol::PACKET_UBX_PVT, 1); // enable PVT packets
  ubx.setpacketrate(UBXProtocol::PACKET_UBX_SAT, 1); // enable SAT packets
  ubx.reset(UBXProtocol::RESET_COLD);                // cold reset

  gps.begin(GPSSerial);
}

void loop()
{
    static unsigned long last;
    static int max_seen = 0;
    if (millis() / 10000 != last / 10000)
    {
        last = millis();
        auto & sats = gps.GetSatellites();
        auto & snap = gps.GetSnapshot();
        int used = 0;
        for (auto sat : sats)
            if (sat.used) ++used;

        Serial.printf("%s %d %c %2d/%02d: ", DEVNAME, last / 1000, (char)snap.FixStatus.Value(), used, sats.size());
        for (int i=0; i<sats.size(); ++i)
            Serial.print(i < used ? '*' : '-');
        if (max_seen < sats.size())
            max_seen = sats.size();
        for (int i=sats.size(); i<max_seen; ++i)
            Serial.print(" ");
        
        Serial.printf(" Max = %d\r\n", max_seen);
    }
}
