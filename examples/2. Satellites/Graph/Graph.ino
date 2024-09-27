#include <TinyGPSAsync.h>
#if __has_include("../examples/Device.h")
    #include "../examples/Device.h"
#endif

TinyGPSAsync gps;
unsigned long sketch_start;
#define GREEN_COLOUR (USE_ANSI_COLOURING ? "\033[32m" : "")
#define BLUE_COLOUR (USE_ANSI_COLOURING ? "\033[34m" : "")
#define DEFAULT_COLOUR (USE_ANSI_COLOURING ? "\033[0m" : "")

void setSystemTime(int year, int month, int day, int hour, int minute, int second)
{
    struct tm t;
    t.tm_year = year - 1900; // Year since 1900
    t.tm_mon = month - 1;    // Month, where 0 = January
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;
    time_t timeSinceEpoch = mktime(&t); // Convert to time_t (seconds since epoch)
    
    struct timeval now = { .tv_sec = timeSinceEpoch };
    settimeofday(&now, NULL); // Set the system clock
}

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

  sketch_start = millis();
}

void loop()
{
    static uint max_seen = 0;
    static uint last_seen = 0;
    static uint last_used = 0;
    static bool last_hasFix = false;
    static uint last_seconds = 0;
    static bool ever_got_time = false;

    uint seconds = (millis() - sketch_start) / 1000;

    if (seconds != last_seconds || gps.NewSatellitesAvailable())
    {
        auto & sats = gps.GetSatellites();
        int used = 0;
        for (auto sat : sats)
            if (sat.used) ++used;
        auto & snap = gps.GetSnapshot();
        bool hasFix = snap.FixStatus.Value() == 'A';
        bool somethingChanged = used != last_used || sats.size() != last_seen || hasFix != last_hasFix;
        bool drawSomething = seconds != last_seconds || somethingChanged;
        if (!snap.Time.IsVoid())
        {
            ever_got_time = true;
            setSystemTime(2024, 1, 1, snap.Time.Hour(), snap.Time.Minute(), snap.Time.Second());
        }

        if (somethingChanged)
        {
            Serial.println();
            last_used = used;
            last_seen = sats.size();
            last_hasFix = hasFix;
        }

        if (drawSomething)
        {
            last_seconds = seconds;
            if (ever_got_time)
            {
                time_t t = time(NULL);
                auto time = localtime(&t);
                Serial.printf("%s%02d:%02d:%02d%s ", GREEN_COLOUR,
                    time->tm_hour, time->tm_min, time->tm_sec,
                    DEFAULT_COLOUR);
            }
            else
            {
                Serial.printf("%02d:%02d:%02d ", seconds / 3600, (seconds / 60) % 60, seconds % 60);
            }

            Serial.printf("Used=%2d Seen=%2d: ", used, sats.size());

            // Render the sat info in graphical form
            if (hasFix) Serial.printf(GREEN_COLOUR);
            for (int i=0; i<used; ++i) 
                Serial.print(hasFix ? '*' : '+' );
            if (hasFix) Serial.printf(DEFAULT_COLOUR);            
            for (int i=used; i<sats.size(); ++i)
                Serial.print('-');
            if (max_seen < sats.size())
                max_seen = sats.size();
            for (int i=sats.size(); i<max_seen; ++i)
                Serial.print(" ");
            Serial.printf(" Max = %d\r", max_seen);
        }
    }
}
