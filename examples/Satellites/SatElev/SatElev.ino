#include <Arduino.h>
#include <TinyGPSAsync.h>
#include <map>

TinyGPSAsync gps;

/* Here's where you customize for your personal ESP32 setup */
#define GPS_RX_PIN D0
#define GPS_TX_PIN D1
#define GPS_BAUD 9600
#define GPSSerial Serial1

static const int MAX_SATELLITES = 60;
struct satelevinfo
{
  bool changed = false;
  int elev = -1;
};
std::map<string, vector<satelevinfo>> data;

void setup()
{
  Serial.begin(115200);
  GPSSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  delay(2000); // Allow ESP32 serial to initialize

  Serial.println("SatElev.ino");
  Serial.println("Examining GSV satellite data");
  Serial.println();

  gps.begin(GPSSerial);
}

int ElevFromSat(const Satellites &sats, int satno)
{
  for (int i=0; i<sats.Sats.size(); ++i)
    if (sats.Sats[i].prn == satno)
      return (int)sats.Sats[i].elevation;
  return -1;
}

void Header()
{
  Serial.println();
  Serial.printf("         Id Sat #");
  for (int i=0; i<MAX_SATELLITES; ++i)
    Serial.printf("%02d ", i);
  Serial.println(); Serial.flush();
  Serial.printf("-----------------");
  for (int i=0; i<MAX_SATELLITES; ++i)
    Serial.printf("---");
  Serial.println(); Serial.flush();
}

void loop()
{
#if true
static string lastid = "";
static string lastno = "";
  if (gps.NewSentenceAvailable())
  {
    auto &sent = gps.GetSentences();
    Serial.printf("%s%s\n", sent.LastSentence.SentenceId() == "GSV" ? "---" : "   ", sent.LastSentence.String().c_str());
    if (sent.LastSentence.SentenceId() == "GSV")
    {
      bool printit = false;
      string id = sent.LastSentence.TalkerId();
      string index = sent.LastSentence[2];

      if (lastid == "BD")
        printit = (id != "GP" || index != "1") && (id != "BD" || index != "1");
      else if (lastid == "GP")
        printit = (lastno == "1" && index != "2") || (lastno == "2" && index != "3") || (lastno == "3" && (id != "BD" || index != "1"));
      else
        printit = true;
      if (printit)
      {
        Serial.printf("**********Last = %s/%s\n", lastid.c_str(), lastno.c_str());
        Serial.printf("**********%s: %s/%s\n\n", id.c_str(), sent.LastSentence[2].c_str(), sent.LastSentence[1].c_str());
      }
      lastid = id;
      lastno = index;
    }
#else
  if (!gps.NewSatellitesAvailable())
    return;
#endif
  }
  return;



  static int linecount = 0;
  bool chg = false;
  bool newsent = gps.NewSentenceAvailable();
  auto & sats = gps.GetSatellites();
  if (data.find(sats.Talker) == data.end())
  {
    data[sats.Talker] = vector<satelevinfo>(MAX_SATELLITES);
  }

  for (int i=0; i<MAX_SATELLITES; ++i)
  {
    int elev = ElevFromSat(sats, i);
    auto & thisSat = data[sats.Talker][i];
    if (elev != thisSat.elev)
    {
      thisSat.changed = chg = true;
      thisSat.elev = elev;
    }
  }
  
  auto &ss = gps.GetSnapshot();
  if (chg)
  {
    if (linecount++ % 20 == 0)
      Header();
    auto t = ss.Time;
    Serial.printf("%02d:%02d:%02d %2s      ", t.Hour(), t.Minute(), t.Second(), sats.Talker.c_str());
    for (int i=0; i<MAX_SATELLITES; ++i)
    {
      auto & thisSat = data[sats.Talker][i];
      char ch = thisSat.changed && linecount != 1 ? '*' : ' ';
      if (thisSat.elev == -1)
        Serial.printf("  %c", ch);
      else
        Serial.printf("%02d%c", thisSat.elev, ch);
      thisSat.changed = false;
    }
#if false
    auto &counters = ss.statistics;
    auto status = gps.DiagnosticCode();
    auto statusString = gps.DiagnosticString();
    Serial.printf("CRx = %d  Snt Val/Inv = %d/%d  Chk p/f = %d/%d  Fix? %c  Status = '%s' (%d)",
      counters.encodedCharCount, counters.validSentenceCount, counters.invalidSentenceCount, counters.passedChecksumCount, counters.failedChecksumCount,
      ss.FixStatus.IsPositionValid() ? 'y' : 'n', statusString.c_str(), status);
#endif
    Serial.println();
  }

#if false
  if (newsent)
  {
    auto &sent = ss.sentences;
    for (auto s : sent.AllSentences)
      if (s.second.SentenceId() == "GSV")
        Serial.println(s.second.String().c_str());
  }
#endif
}
