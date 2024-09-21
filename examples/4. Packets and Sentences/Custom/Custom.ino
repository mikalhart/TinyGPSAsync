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

  Serial.println("Custom.ino");
  Serial.println("Custom extraction of fields from arbitrary sentences");
  Serial.println();

  gps.begin(GPSSerial);
}

// In this example, we'll extract VDOP, which is the 15th field in
// the commonly used GSA sentence
void loop()
{
  auto s = gps.GetSentenceMap();
  auto &gsa = s["GSA"];
  static string oldvdop;
  static int oldstatus = -1;
  string vdop;
  int status;
  bool changed = false;

  // If there's a new GSA sentence, extract VDOP from it
  if (gsa.IsNew())
  {
    vdop = gsa[15];
    if (vdop != oldvdop)
    {
      oldvdop = vdop;
      changed = true;
    }
  }

  status = gps.DiagnosticCode();
  if (status != oldstatus)
  {
    oldstatus = status;
    changed = true;
  }

  if (changed)
  {
    Serial.printf("VDOP: %s Status: %s\n", 
      oldvdop.empty() ? "Unknown" : oldvdop.c_str(), gps.DiagnosticString());
  }
}
