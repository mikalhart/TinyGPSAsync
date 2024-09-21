#include <TinyGPSAsync.h>
#if __has_include("../examples/Device.h")
    #include "../examples/Device.h"
#endif

TinyGPSAsync gps;

void setup()
{
  Serial.begin(115200);

  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  delay(2000); // Allow ESP32 serial to initialize

  Serial.println("Diagnostics.ino");
  Serial.println("Quick troubleshooting in TinyGPSAsync");
  Serial.println();

  gps.begin(GPSSerial);
}

void loop()
{
  int status = gps.DiagnosticCode();
  string statusString = gps.DiagnosticString();
  if (gps.NewPacketAvailable())
  {
    auto &counters = gps.GetStatistics();
    auto &ss = gps.GetSnapshot();
    Serial.printf("Characters Rx = %d  Sentences Valid/Invalid = %d/%d  Chksum pass/fail = %d/%d  Position Fix? %c  Status = '%s' (%d)\n",
      counters.encodedCharCount, counters.validSentenceCount, counters.invalidSentenceCount, counters.passedChecksumCount, counters.failedChecksumCount,
      ss.FixStatus.IsPositionValid() ? 'y' : 'n', statusString.c_str(), status);
  }
}
