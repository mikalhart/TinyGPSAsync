#include "TinyGPSAsync.h"

// static
// Parse degrees in that funny NMEA format DDMM.MMMM
void TinyGPSAsync::LocationItem::parseDegrees(const char *degTerm, const char *nsewTerm, TinyGPSAsync::LocationItem::Raw::RawD &deg)
{
    uint32_t leftOfDecimal = (uint32_t)atol(degTerm);
    uint16_t minutes = (uint16_t)(leftOfDecimal % 100);
    uint32_t multiplier = 10000000UL;
    uint32_t tenMillionthsOfMinutes = minutes * multiplier;

    deg.deg = (int16_t)(leftOfDecimal / 100);

    while (isdigit(*degTerm))
        ++degTerm;

    if (*degTerm == '.')
        while (isdigit(*++degTerm))
        {
            multiplier /= 10;
            tenMillionthsOfMinutes += (*degTerm - '0') * multiplier;
        }

    deg.billionths = (5 * tenMillionthsOfMinutes + 1) / 3;
    deg.negative = *nsewTerm == 'W' || *nsewTerm == 'S';
}

void TinyGPSAsync::DecimalItem::parse(const char *term)
{
    bool negative = *term == '-';
    if (negative) ++term;
    int32_t ret = 100 * (int32_t)atol(term);
    while (isdigit(*term)) ++term;
    if (*term == '.' && isdigit(term[1]))
    {
        ret += 10 * (term[1] - '0');
        if (isdigit(term[2]))
            ret += term[2] - '0';
    }
    val = negative ? -ret : ret;
}


TinyGPSAsync::LocationItem::Dbl TinyGPSAsync::LocationItem::Get()
{
    process();
    Dbl d;
    d.Lat = r.Lat.deg + r.Lat.billionths / 1000000000.0;
    if (r.Lat.negative)
        d.Lat = -d.Lat;
    d.Lng = r.Lng.deg + r.Lng.billionths / 1000000000.0;
    if (r.Lng.negative)
        d.Lng = -d.Lng;
    isNew = false;
    return d;
}

TinyGPSAsync::TimeItem::Time TinyGPSAsync::TimeItem::Get() 
{ 
    process(); 
    Time time;
    time.Hour = val / 1000000;
    time.Minute = (val / 10000) % 100;
    time.Second = (val / 100) % 100;
    time.CentiSecond = val % 100;
    isNew = false;
    return time;
}

TinyGPSAsync::DateItem::Date TinyGPSAsync::DateItem::Get() 
{ 
    process(); 
    Date date;
    date.Year = d % 100 + 2000;
    date.Month = (d / 100) % 100;
    date.Day = d / 10000;
    isNew = false;
    return date;
}


/* static */
TinyGPSAsync::ParsedSentence TinyGPSAsync::ParsedSentence::FromString(string str)
{
    ParsedSentence s;
    s.lastUpdateTime = millis();
    s.isNew = true;
    s.charCount = str.length();
    size_t start = 0;
    size_t delimiterPos = str.find_first_of(",*"); 
    unsigned long calculatedChksum = 0;
    log_d("Parsing sentence %s", str.c_str());
    while (true)
    {
        string token = delimiterPos != string::npos ? str.substr(start, delimiterPos - start) : str.substr(start);
        s.fields.push_back(token);
        for (auto c : token.substr(s.fields.size() == 1 && s.fields[0].substr(0, 1) == "$" ? 1 : 0))
            calculatedChksum ^= c;

        if (delimiterPos == string::npos)
        {
            s.hasChecksum = s.checksumValid = false;
            log_d("Doesn't have any * checksum");
            break;
        }

        if (str[delimiterPos] == '*')
        {
            token = str.substr(delimiterPos + 1);
            s.fields.push_back(token);
            if (token.find_first_not_of("0123456789ABCDEFabcdef") != string::npos)
            {
                s.hasChecksum = s.checksumValid = false;
                log_e("Bad format checksum: '%s'", token.c_str());
                log_e("Sentence was '%s'", str.c_str());
            }
            else
            {
                unsigned long suppliedChecksum = strtoul(token.c_str(), NULL, 16);
                s.hasChecksum = true;
                s.checksumValid = suppliedChecksum == calculatedChksum;
            }
            break;
        }

        calculatedChksum ^= ',';
        start = delimiterPos + 1;
        delimiterPos = str.find_first_of(",*", start);
    }

    return s;
}

string TinyGPSAsync::ParsedSentence::Get()
{
    string str;
    for (int i=0; i<fields.size(); ++i)
    {
        str += fields[i];
        if (i != fields.size() - 1)
            str += i == fields.size() - 2 ? '*' : ',';
    }
    return str;
}

void TinyGPSAsync::processGGA(TinyGPSAsync::ParsedSentence &sentence)
{
    if (sentence[1].length() > 0)
    {
        Time.parse(sentence[1].c_str());
        Time.everUpdated = Time.isNew = true;
        Time.lastUpdateTime = sentence.Timestamp();
    }

    if (sentence[2].length() > 0 && sentence[4].length() > 0)
    {
        Location.parseDegrees(sentence[2].c_str(), sentence[3].c_str(), Location.r.Lat);
        Location.parseDegrees(sentence[4].c_str(), sentence[5].c_str(), Location.r.Lng);
        Location.everUpdated = Location.isNew = true;
        Location.lastUpdateTime = sentence.Timestamp();
    }

    if (sentence[6].length() > 0)
    {
        Quality.parse(sentence[6].c_str());
        Quality.everUpdated = Quality.isNew = true;
        Quality.lastUpdateTime = sentence.Timestamp();
    }

    if (sentence[7].length() > 0)
    {
        SatelliteCount.parse(sentence[7].c_str());
        SatelliteCount.everUpdated = SatelliteCount.isNew = true;
        SatelliteCount.lastUpdateTime = sentence.Timestamp();
    }

    if (sentence[8].length() > 0)
    {
        HDOP.parse(sentence[8].c_str());
        HDOP.everUpdated = HDOP.isNew = true;
        HDOP.lastUpdateTime = sentence.Timestamp();
    }

    if (sentence[9].length() > 0)
    {
        Altitude.parse(sentence[9].c_str());
        Altitude.everUpdated = Altitude.isNew = true;
        Altitude.lastUpdateTime = sentence.Timestamp();
    }
}

void TinyGPSAsync::processRMC(TinyGPSAsync::ParsedSentence &sentence)
{
    if (sentence[1].length() > 0)
    {
        Time.parse(sentence[1].c_str());
        Time.everUpdated = Time.isNew = true;
        Time.lastUpdateTime = sentence.Timestamp();
    }

    if (sentence[2].length() > 0)
    {
        FixStatus.parse(sentence[2].c_str());
        FixStatus.everUpdated = FixStatus.isNew = true;
        FixStatus.lastUpdateTime = sentence.Timestamp();
    }

    if (sentence[3].length() > 0 && sentence[5].length() > 0)
    {
        Location.parseDegrees(sentence[3].c_str(), sentence[4].c_str(), Location.r.Lat);
        Location.parseDegrees(sentence[5].c_str(), sentence[6].c_str(), Location.r.Lng);
        Location.everUpdated = Location.isNew = true;
        Location.lastUpdateTime = sentence.Timestamp();
    }

    if (sentence[7].length() > 0)
    {
        Speed.parse(sentence[7].c_str());
        Speed.everUpdated = Speed.isNew = true;
        Speed.lastUpdateTime = sentence.Timestamp();
    }

    if (sentence[8].length() > 0)
    {
        Course.parse(sentence[8].c_str());
        Course.everUpdated = Course.isNew = true;
        Course.lastUpdateTime = sentence.Timestamp();
    }
    
    if (sentence[9].length() > 0)
    {
        Date.parse(sentence[9].c_str());
        Date.everUpdated = Date.isNew = true;
        Date.lastUpdateTime = sentence.Timestamp();
    }
}

void TinyGPSAsync::process()
{
    if (task.hasNewSentence)
    {
        task.hasNewSentence = false;
        std::map<string, ParsedSentence> newSentences;
        uint32_t now = millis();
        log_d("Taking semaphore");
        if (xSemaphoreTake(task.gpsMutex, portMAX_DELAY) == pdTRUE)
        {
            // Step 1: Copy over any new satellite info from GSV
            if (!task.AllSatellites.empty())
            {
                Satellites.s = task.AllSatellites;
                Satellites.isNew = true;
                Satellites.everUpdated = true;
                Satellites.lastUpdateTime = now;
            }

            // Step 2: Copy over all new sentences
            this->Sentence.LastSentence = task.LastSentence;
            newSentences = task.AllSentences;

            // Step 3: Copy diags
            Diagnostic.Counters.encodedCharCount += task.Counters.encodedCharCount;
            Diagnostic.Counters.validSentenceCount += task.Counters.validSentenceCount;
            Diagnostic.Counters.failedChecksumCount += task.Counters.failedChecksumCount;
            Diagnostic.Counters.passedChecksumCount += task.Counters.passedChecksumCount;
            Diagnostic.Counters.invalidSentenceCount += task.Counters.invalidSentenceCount;
            Diagnostic.Counters.ggaCount += task.Counters.ggaCount;
            Diagnostic.Counters.rmcCount += task.Counters.rmcCount;
            Diagnostic.isNew = true;
            Diagnostic.everUpdated = true;
            Diagnostic.lastUpdateTime = now;

            task.Clear();
            xSemaphoreGive(task.gpsMutex);
        }

        // Step 4: After the semaphore has been released, process all new sentences
        for (auto kvp : newSentences)
        {
            Sentence.AllSentences[kvp.first] = kvp.second;
            Sentence.isNew = true;
            if (kvp.second.ChecksumValid())
            {
                if (kvp.first == "GGA")
                    processGGA(kvp.second);
                else if (kvp.first == "RMC")
                    processRMC(kvp.second);
            }
        }
    }
}

/* static */
const char *TinyGPSAsync::Cardinal(double course)
{
  static const char* directions[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  int direction = (int)((course + 11.25f) / 22.5f);
  return directions[direction % 16];
}

/* static */
double TinyGPSAsync::DistanceBetween(double lat1, double long1, double lat2, double long2)
{
  // returns distance in meters between two positions, both specified
  // as signed decimal-degrees latitude and longitude. Uses great-circle
  // distance computation for hypothetical sphere of radius 6372795 meters.
  // Because Earth is no exact sphere, rounding errors may be up to 0.5%.
  // Courtesy of Maarten Lamers
  double delta = radians(long1-long2);
  double sdlong = sin(delta);
  double cdlong = cos(delta);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double slat1 = sin(lat1);
  double clat1 = cos(lat1);
  double slat2 = sin(lat2);
  double clat2 = cos(lat2);
  delta = (clat1 * slat2) - (slat1 * clat2 * cdlong);
  delta = sq(delta);
  delta += sq(clat2 * sdlong);
  delta = sqrt(delta);
  double denom = (slat1 * slat2) + (clat1 * clat2 * cdlong);
  delta = atan2(delta, denom);
  return delta * _GPS_EARTH_RADIUS;
}

/* static */
double TinyGPSAsync::CourseTo(double lat1, double long1, double lat2, double long2)
{
  // returns course in degrees (North=0, West=270) from position 1 to position 2,
  // both specified as signed decimal-degrees latitude and longitude.
  // Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
  // Courtesy of Maarten Lamers
  double dlon = radians(long2-long1);
  lat1 = radians(lat1);
  lat2 = radians(lat2);
  double a1 = sin(dlon) * cos(lat2);
  double a2 = sin(lat1) * cos(lat2) * cos(dlon);
  a2 = cos(lat1) * sin(lat2) - a2;
  a2 = atan2(a1, a2);
  if (a2 < 0.0)
  {
    a2 += TWO_PI;
  }
  return degrees(a2);
}

int TinyGPSAsync::DiagnosticItem::Status()
{
    process();
    if (pThis->task.stream == nullptr)
        return STREAM;
    if (millis() - pThis->startTime < 10000)
        return WAIT;
    if (Counters.encodedCharCount < 10)
        return WIRING;
    if (Counters.invalidSentenceCount > 3 || Counters.validSentenceCount == 0)
        return BAUD;
    if (Counters.failedChecksumCount > 3)
        return OVERFLOW;
    if (!pThis->FixStatus.IsPositionValid())
        return FIX;
    if (Counters.ggaCount < 5 || Counters.rmcCount < 5)
        return MISSING;

    return OK;
}

string TinyGPSAsync::DiagnosticItem::StatusString(int status)
{
    switch(status)
    {
        case STREAM:
            return "Parser not started";
        case WAIT:
            return "Status not yet available";
        case WIRING:
            return "Possible wiring issue";
        case BAUD:
            return "Possible baud rate mismatch";
        case OVERFLOW:
            return "Characters dropping";
        case FIX:
            return "No position fix yet";
        case MISSING:
            return "GGA or RMC not available";
    }
    return "Ok";
}
