#include "TinyGPSAsync.h"

// Parse degrees in that funny NMEA format DDMM.MMMM
void Snapshot::LocationItem::LocationAngle::parseDegrees(const char *degTerm, const char *nsewTerm)
{
    uint32_t leftOfDecimal = (uint32_t)atol(degTerm);
    uint16_t minutes = (uint16_t)(leftOfDecimal % 100);
    uint32_t multiplier = 10000000UL;
    uint32_t tenMillionthsOfMinutes = minutes * multiplier;

    this->deg = (int16_t)(leftOfDecimal / 100);

    while (isdigit(*degTerm))
        ++degTerm;

    if (*degTerm == '.')
        while (isdigit(*++degTerm))
        {
            multiplier /= 10;
            tenMillionthsOfMinutes += (*degTerm - '0') * multiplier;
        }

    this->billionths = (5 * tenMillionthsOfMinutes + 1) / 3;
    this->negative = *nsewTerm == 'W' || *nsewTerm == 'S';
}

void Snapshot::DecimalItem::parse(const string &term, uint32_t timestamp)
{
    stampIt(timestamp);
    isVoid = term.empty();
    if (!isVoid)
    {
        const char *t = term.c_str();
        bool negative = *t == '-';
        if (negative)
            ++t;
        int32_t ret = 100 * (int32_t)atol(t);
        while (isdigit(*t))
            ++t;
        if (*t == '.' && isdigit(t[1]))
        {
            ret += 10 * (t[1] - '0');
            if (isdigit(t[2]))
                ret += t[2] - '0';
        }
        v = negative ? -ret : ret;
    }
}

void TinyGPSAsync::processGGA(const NmeaPacket &sentence)
{
    auto timestamp = sentence.Timestamp();
    snapshot.Time.parse(sentence[1].c_str(), timestamp);
    snapshot.Location.parse(sentence[2], sentence[3], sentence[4], sentence[5], timestamp);
    snapshot.Quality.parse(sentence[6], timestamp);
    snapshot.SatelliteCount.parse(sentence[7], timestamp);
    snapshot.HDOP.parse(sentence[8].c_str(), timestamp);
    snapshot.Altitude.parse(sentence[9].c_str(), timestamp);
}

void TinyGPSAsync::processRMC(const NmeaPacket &sentence)
{
    auto timestamp = sentence.Timestamp();
    snapshot.Time.parse(sentence[1].c_str(), timestamp);
    snapshot.FixStatus.parse(sentence[2].c_str(), timestamp);
    snapshot.Location.parse(sentence[3], sentence[4], sentence[5], sentence[6], timestamp);
    snapshot.Speed.parse(sentence[7].c_str(), timestamp);
    snapshot.Course.parse(sentence[8].c_str(), timestamp);
    snapshot.Date.parse(sentence[9], timestamp);
}

void TinyGPSAsync::processUbxNavPvt(const UbxPacket &pu)
{
    auto & payload = pu.Packet().payload;
    auto timestamp = pu.Timestamp();
    if (payload.size() == 92)
    {
        uint8_t validityFlags = payload[11];
        uint8_t flagsFlags = payload[21];
        if (validityFlags & 0x1) // date valid
        {
            uint16_t year = makeU16(payload[4], payload[5]);
            uint8_t month = payload[6];
            uint8_t day = payload[7];
            snapshot.Date.set(year, month, day, timestamp);
        }
        else
        {
            snapshot.Date.clear(timestamp);
        }

        if (validityFlags & 0x2) // time valid
        {
            uint8_t hour = payload[8];
            uint8_t minute = payload[9];
            uint8_t second = payload[10];
            int32_t nanos = makeI32(payload[16], payload[17], payload[18], payload[19]);
            snapshot.Time.set(hour, minute, second, nanos, timestamp);
        }
        else
        {
            snapshot.Time.clear(timestamp);
        }

        if (flagsFlags & 0x1) // location fix valid
        {
            int32_t lat = makeI32(payload[28], payload[29], payload[30], payload[31]);
            int32_t lng = makeI32(payload[24], payload[25], payload[26], payload[27]);
            snapshot.Location.set(lat, lng, timestamp);
            snapshot.FixStatus.set(Snapshot::FixStatusItem::A, timestamp);
        }
        else
        {
            snapshot.Location.clear(timestamp);
            snapshot.FixStatus.set(Snapshot::FixStatusItem::V, timestamp);
        }

        // Quality
        // Sat count
        // HDOP
        // Altitude
        // Speed
        // Course
    }
}


void TinyGPSAsync::syncStatistics()
{
    if (task.hasNewPacket)
    {
        log_d("Taking semaphore");
        if (xSemaphoreTake(task.gpsMutex, portMAX_DELAY) == pdTRUE)
        {
            // Copy statistics
            statistics.encodedCharCount += task.Counters.encodedCharCount;
            statistics.validSentenceCount += task.Counters.validSentenceCount;
            statistics.failedChecksumCount += task.Counters.failedChecksumCount;
            statistics.passedChecksumCount += task.Counters.passedChecksumCount;
            statistics.invalidSentenceCount += task.Counters.invalidSentenceCount;
            statistics.ggaCount += task.Counters.ggaCount;
            statistics.rmcCount += task.Counters.rmcCount;
            statistics.ubxNavSatCount += task.Counters.ubxNavSatCount;
            statistics.ubxNavPvtCount += task.Counters.ubxNavPvtCount;
            task.Counters.clear();
            xSemaphoreGive(task.gpsMutex);
        }
    }
}

void TinyGPSAsync::syncSatellites()
{
    if (task.hasNewSatellites)
    {
        log_d("Taking semaphore");
        if (xSemaphoreTake(task.gpsMutex, portMAX_DELAY) == pdTRUE)
        {
            // Copy over any new satellite info from GSV
            if (!task.AllSatellites.empty())
            {
                satellites = task.AllSatellites;
            }

            task.AllSatellites.clear();
            xSemaphoreGive(task.gpsMutex);
        }
    }
}

void TinyGPSAsync::syncPackets()
{
    if (task.hasNewPacket)
    {
        std::map<string, NmeaPacket> newSentences;
        std::map<string, UbxPacket> newUbxPackets;
        log_d("Taking semaphore");
        if (xSemaphoreTake(task.gpsMutex, portMAX_DELAY) == pdTRUE)
        {
            if (task.hasNewSentences)
            {
                // Copy over all new sentences
                latestNmea = task.LastSentence;
                newSentences = task.NewSentences;
                task.hasNewSentences = false;
                task.NewSentences.clear();
            }

            if (task.hasNewUbxPackets)
            {
                latestUbx = task.LastUbxPacket;
                newUbxPackets = task.NewUbxPackets;
                task.hasNewUbxPackets = false;
                task.NewUbxPackets.clear();
            }

            if (task.hasNewUnknownPacket)
            {
                latestUnknown = task.LastUnknownPacket;
                task.hasNewUnknownPacket = false;
                xSemaphoreGive(task.gpsMutex);
            }
            if (task.lastPacketType == TaskSpecific::NMEA)
                latest = &latestNmea;
            else if (task.lastPacketType == TaskSpecific::UBX)
                latest = &latestUbx;
            else
                latest = &latestUnknown;
            xSemaphoreGive(task.gpsMutex);
        }

        // After the semaphore has been released, process all new sentences
        for (auto kvp : newSentences)
            sentenceMap.AllSentences[kvp.first] = kvp.second;
        for (auto kvp : newUbxPackets)
            ubxPacketMap.AllUbxPackets[kvp.first] = kvp.second;
    }
}

void TinyGPSAsync::syncSnapshot()
{
    if (task.hasNewSnapshot)
    {
        std::map<string, NmeaPacket> newSentences;
        std::map<string, UbxPacket> newUbxPackets;
        log_d("Taking semaphore");
        if (xSemaphoreTake(task.gpsMutex, portMAX_DELAY) == pdTRUE)
        {
            // Copy over all new sentences and packets
            newSentences = task.SnapshotSentences;
            task.SnapshotSentences.clear();
            newUbxPackets = task.SnapshotUbxPackets;
            task.SnapshotUbxPackets.clear();
            xSemaphoreGive(task.gpsMutex);
        }

        // After the semaphore has been released, process all new sentences and packets
        for (auto kvp : newSentences)
        {
            if (kvp.second.ChecksumValid())
            {
                if (kvp.first == "GGA")
                    processGGA(kvp.second);
                else if (kvp.first == "RMC")
                    processRMC(kvp.second);
            }
        }

        for (auto kvp : newUbxPackets)
        {
            if (kvp.second.ChecksumValid())
            {
                processUbxNavPvt(kvp.second);
            }
        }
    }
}

/* static */
const char *TinyGPSAsync::Cardinal(double course)
{
    static const char *directions[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE", "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
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
    double delta = radians(long1 - long2);
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
    return delta * Snapshot::_GPS_EARTH_RADIUS;
}

/* static */
double TinyGPSAsync::CourseTo(double lat1, double long1, double lat2, double long2)
{
    // returns course in degrees (North=0, West=270) from position 1 to position 2,
    // both specified as signed decimal-degrees latitude and longitude.
    // Because Earth is no exact sphere, calculated course may be off by a tiny fraction.
    // Courtesy of Maarten Lamers
    double dlon = radians(long2 - long1);
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

TinyGPSAsync::Status TinyGPSAsync::DiagnosticCode()
{
    if (task.GetStream() == nullptr)
        return STREAM;
    auto &stats = GetStatistics();
    if (Utils::msticks() - startTime >= 5000)
    {
        if (stats.encodedCharCount < 10)
            return WIRING;
        if (stats.invalidSentenceCount > 3 || stats.validSentenceCount == 0)
            return BAUD;
        if (stats.ggaCount < 5 && stats.rmcCount < 5 && stats.ubxNavPvtCount < 5)
            return MISSING;
    }
    if (stats.failedChecksumCount > 3)
        return OVERFLOW;
    return OK;
}

const char *TinyGPSAsync::DiagnosticString()
{
    static std::map<TinyGPSAsync::Status, const char *> myMap =
    {
        { STREAM, "Parser not started" },
        { WIRING, "Possible wiring issue" },
        { BAUD, "Possible baud rate mismatch" },
        { OVERFLOW, "Characters dropping" },
        { MISSING, "Snapshot UBX/NMEA not available" },
        { OK, "Ok" }
    };
    Status status = DiagnosticCode();
    return myMap.find(status) != myMap.end() ? myMap[status] : "Unknown";
}