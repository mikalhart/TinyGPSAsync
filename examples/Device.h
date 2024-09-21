/* Here's where you customize for your personal ESP32 setup */
#define GPS_RX_PIN D1
#define GPS_TX_PIN D0

#if false // Quescan
#define GPS_BAUD 38400
#define GPSSerial Serial1
#define DEVNAME "QUESC"
#else
#define GPS_BAUD 9600
#define GPSSerial Serial1
#define DEVNAME "SAM10"
#endif


#if false // L80-R
#define RX D0
#define TX D1
#define GPSBAUD 9600
// #define COLDRESETBYTES "$PMTK103*30\r\n" // second coldest
#define GPSNAME "L80-R"
#define COLDRESETBYTES "$PMTK104*37\r\n"
#define WARMRESETBYTES "$PMTK102*31\r\n"
#define HOTRESETBYTES  "$PMTK101*32\r\n"
#define isValidDateTime(d, t) (d.Year() != 2080 || d.Month() != 1)
#endif
