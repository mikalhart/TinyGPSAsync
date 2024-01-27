#include <Arduino.h>
#include <TinyGPSAsync.h>

TinyGPSAsync gps;

/* Here's where you customize for your personal ESP32 setup */
#define GPS_RX_PIN 2
#define GPS_TX_PIN 3
#define GPS_BAUD 9600
#define GPSSerial Serial1

void setup()
{
  Serial.begin(115200);
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  delay(2000); // Allow ESP32 serial to initialize

vector<uint8_t> Navrate10hz = {0xB5, 0x62, 0x06, 0x08, 0x06, 0x00, 0x64, 0x00, 0x01, 0x00, 0x01, 0x00, 0x7A, 0x12};
for (auto c : Navrate10hz)
  GPSSerial.write(c);

  Serial.println("Diagnostics.ino");
  Serial.println("Quick troubleshooting in TinyGPSAsync");
  Serial.println();

  gps.begin(GPSSerial);
}

void loop()
{
  int status = gps.Diagnostic.Status();
  string statusString = gps.Diagnostic.StatusString(status);
  auto counters = gps.Diagnostic.Get();
  Serial.printf("Characters Rx = %d  Sentences Valid/Invalid = %d/%d  Chksum pass/fail = %d/%d  Position Fix? %c  Status = '%s' (%d)\n",
    counters.encodedCharCount, counters.validSentenceCount, counters.invalidSentenceCount, counters.passedChecksumCount, counters.failedChecksumCount,
    gps.FixStatus.IsPositionValid() ? 'y' : 'n', statusString.c_str(), status);

  delay(1000);
}
