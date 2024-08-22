#include <Arduino.h>
#include <map>
#include <vector>
#include <atomic>

#if !defined(ESP32)
#error "This library only works with ESP32"
#endif

#include "esp_task_wdt.h"
using namespace std;

#define _GPS_VERSION "0.0.2" // software version of this library

class TinyGPSAsync
{
private:
    static constexpr double _GPS_MPH_PER_KNOT = 1.15077945;
    static constexpr double _GPS_MPS_PER_KNOT = 0.51444444;
    static constexpr double _GPS_KMPH_PER_KNOT = 1.852;
    static constexpr double _GPS_MILES_PER_METER = 0.00062137112;
    static constexpr double _GPS_KM_PER_METER = 0.001;
    static constexpr double _GPS_FEET_PER_METER = 3.2808399;
    static constexpr double _GPS_EARTH_RADIUS = 6372795;

    struct Item
    {
        friend TinyGPSAsync;
    protected:
        mutable uint32_t lastUpdateTime = 0;
        mutable bool everUpdated = false;
        mutable bool isNew = false;
        void StampIt(uint32_t timestamp) { everUpdated = isNew = true; lastUpdateTime = timestamp; }

    public:
        bool     IsNew() const       { return isNew; }
        bool     EverUpdated() const { return everUpdated; }
        uint32_t Age() const         { return everUpdated ? millis() - lastUpdateTime : ULONG_MAX; }
        uint32_t Timestamp() const   { return lastUpdateTime; }
    };

public:
    class ParsedSentence
    {
    private:
        vector<string> fields;
        bool hasChecksum = false;
        bool checksumValid = false;
        mutable bool isNew = false;
        uint32_t lastUpdateTime = 0;
        uint8_t charCount = 0;
        
    public:
        bool IsValid() const            { return fields.size() > 0  && fields[0].length() == 6 && fields[0][0] == '$' && hasChecksum; }
        bool ChecksumValid() const      { return checksumValid; }
        bool IsNew() const              { bool temp = isNew; isNew = false; return temp; }
        uint32_t Age() const            { return millis() - lastUpdateTime; }
        uint32_t Timestamp() const      { return lastUpdateTime; }
        string TalkerId() const         { return IsValid() ? fields[0].substr(1, 2) : ""; }
        string SentenceId() const       { return IsValid() ? fields[0].substr(3) : ""; }
        void Clear()                    { fields.clear(); }
        string operator[](int field) const   { return field >= fields.size() ? "" : fields[field]; }
        uint8_t CharCount() const       { return charCount; }

        static ParsedSentence FromString(const string &str);
        string String() const;
    };

    class Sentences
    {
    
        friend TinyGPSAsync;
    public:
        ParsedSentence LastSentence;
        std::map<string, ParsedSentence> AllSentences;
        const ParsedSentence &operator[] (const char *id) const { return AllSentences.at(id); }
    };

    struct Statistics
    {
        friend TinyGPSAsync;
        uint32_t encodedCharCount = 0;
        uint32_t validSentenceCount = 0;
        uint32_t invalidSentenceCount = 0;
        uint32_t failedChecksumCount = 0;
        uint32_t passedChecksumCount = 0;
        uint32_t ggaCount = 0;
        uint32_t rmcCount = 0;

        void Clear()
        {
            encodedCharCount = validSentenceCount = invalidSentenceCount = failedChecksumCount =
                passedChecksumCount = ggaCount = rmcCount = 0;
        }
    };

    struct SatInfo
    {
        uint16_t prn;
        uint8_t elevation;
        uint16_t azimuth;
        uint8_t snr;
    };
    
    class Satellites
    {
        friend TinyGPSAsync;
    public:
        std::vector<SatInfo> Sats;
        std::string Talker;
    };

    class Snapshot
    {
        friend TinyGPSAsync;
    public:
        Satellites satellites;
        Sentences sentences;
        Statistics statistics;
        struct LocationItem : public Item
        {
            friend TinyGPSAsync;
        private:
            struct LocationAngle
            {
                uint16_t deg = 0;
                uint32_t billionths = 0;
                bool negative = false;
                void parseDegrees(const char *degTerm, const char *nsewTerm);
            } RawLat, RawLng;
        public:
            double Lat() const
            {
                isNew = false;
                double ret = RawLat.deg + RawLat.billionths / 1000000000.0;
                return RawLat.negative ? -ret : ret;
            }
            double Lng() const
            {
                isNew = false;
                double ret = RawLng.deg + RawLng.billionths / 1000000000.0;
                return RawLng.negative ? -ret : ret;
            }
        };

        struct QualityItem : public Item
        {
            friend TinyGPSAsync;
        public:
            enum QualityEnum { Invalid = '0', GPS = '1', DGPS = '2', PPS = '3', RTK = '4', FloatRTK = '5', Estimated = '6', Manual = '7', Simulated = '8' };
            QualityEnum Value() const { isNew = false; return v; }
        private:
            QualityEnum v = Invalid;
            void parse(const char *term) { v = (QualityEnum)*term; }
        };

        struct FixStatusItem : public Item
        {
            friend TinyGPSAsync;
        public:
            enum StatusEnum { N = 'N', V = 'V' , A = 'A'};
            StatusEnum Value() const { isNew = false; return v; }
            bool IsPositionValid() const { isNew = false; return v == A; }
        private:
            StatusEnum v = N;
            void parse(const char *term) { v = (StatusEnum)*term; }
        };

        struct IntegerItem : public Item
        {
            friend TinyGPSAsync;
        protected:
            uint32_t v = 0;
            void parse(const char *term) { v = atol(term); }
        public:
            uint32_t Value() const { isNew = false; return v; }
        };

        struct DateItem : public IntegerItem // todo fix parse once
        {
        public:
            uint16_t Year() const { isNew = false; return v % 100 + 2000; }
            uint8_t Month() const { isNew = false; return (v / 100) % 100; }
            uint8_t Day() const   { isNew = false; return v / 10000; }
        };

        struct DecimalItem : public Item
        {
            friend TinyGPSAsync;
        protected:
            int32_t v = 0;
            void parse(const char *term);
        public:
            int32_t Value() const { isNew = false; return v; }
        };

        struct TimeItem : public DecimalItem // todo set this at parse time
        {
        public:
            uint8_t Hour() const        { isNew = false; return v / 1000000; }
            uint8_t Minute() const      { isNew = false; return (v / 10000) % 100; }
            uint8_t Second() const      { isNew = false; return (v / 100) % 100; }
            uint8_t Centisecond() const { isNew = false; return v % 100; }
        };

        struct SpeedItem : public DecimalItem
        {
            double Knots() const    { isNew = false; return v / 100.0; }
            double Mph() const      { isNew = false; return _GPS_MPH_PER_KNOT * v / 100.0; }
            double Mps() const      { isNew = false; return _GPS_MPS_PER_KNOT * v / 100.0; }
            double Kmph() const     { isNew = false; return _GPS_KMPH_PER_KNOT * v / 100.0; }
        };

        struct CourseItem : public DecimalItem
        {
            double Degrees() const  { isNew = false; return v / 100.0; }
            double Radians() const  { isNew = false; return Degrees() * PI / 180.0; }
        };

        struct AltitudeItem : public DecimalItem
        {
            double Meters() const      { isNew = false; return v / 100.0; }
            double Miles() const       { isNew = false; return _GPS_MILES_PER_METER * v / 100.0; }
            double Kilometers() const  { isNew = false; return _GPS_KM_PER_METER * v / 100.0; }
            double Feet() const        { isNew = false; return _GPS_FEET_PER_METER * v / 100.0; }
        };

        struct HDOPItem : public DecimalItem
        {
            double AsDouble() const    { isNew = false; return v / 100.0; }
        };
    public:
        LocationItem Location;
        DateItem Date;
        TimeItem Time;
        SpeedItem Speed;
        CourseItem Course;
        AltitudeItem Altitude;
        IntegerItem SatelliteCount;
        QualityItem Quality;
        FixStatusItem FixStatus;
        HDOPItem HDOP;
    };

    enum Status {OK = 0, STREAM, WIRING, BAUD, OVERFLOW, MISSING };
    Status DiagnosticCode();
    string DiagnosticString();

    Snapshot snapshot;

public:
    const Snapshot &GetSnapshot() { sync(); return snapshot; }
    const Satellites &GetSatellites() { sync(); return snapshot.satellites;}
    const Sentences &GetSentences() { sync(); return snapshot.sentences; }
    const Statistics &GetStatistics() { sync(); return snapshot.statistics; }
    bool NewSnapshotAvailable() { return task.hasNewSnapshot; }
    bool NewSentenceAvailable() { return task.hasNewSentences; }
    bool NewCharactersAvailable() { return task.hasNewCharacters; }
    bool NewSatellitesAvailable() { return task.hasNewSatellites; }

private:
    uint32_t startTime;

    class TaskSpecific
    {
    public:
        /* For signaling to main thread */
        std::atomic<bool> hasNewCharacters;
        std::atomic<bool> hasNewSatellites;
        std::atomic<bool> hasNewSentences;
        std::atomic<bool> hasNewSnapshot;
        std::atomic<bool> taskActive;

        /* Items guarded by mutex */
        ParsedSentence LastSentence;
        std::map<string, ParsedSentence> AllSentences;
        Statistics Counters;
        vector<SatInfo> AllSatellites;
        string SatelliteTalkerId;

        /* Internal items (not guarded) */
        vector<SatInfo> SatelliteBuffer;
        Stream *stream = nullptr;
        SemaphoreHandle_t gpsMutex = xSemaphoreCreateMutex();

        void processNewSentence(string &s);
        void processGarbageCharacters(int count);
        static void gpsTask(void *pvParameters);

        void Clear()
        {
            Counters.Clear();
            AllSentences.clear();
            AllSatellites.clear();
            SatelliteTalkerId = "";
        }
    } task;

    void sync();
private:
    void processGGA(ParsedSentence &sentence);
    void processRMC(ParsedSentence &sentence);

public:
    void end()
    {
        if (task.taskActive)
        {
            task.taskActive = false;
            vTaskDelay(500);
            task.stream = nullptr;
        }
    }

    bool begin(Stream &stream)
    {
        end();
        task.stream = &stream;
        task.Clear();
        task.taskActive = true;
        startTime = millis();

        xTaskCreatePinnedToCore(task.gpsTask, "gpsTask", 10000, &task, /* tskIDLE_PRIORITY */ uxTaskPriorityGet(NULL), NULL, 0);

        return true;
    }

    static const char *Cardinal(double course);
    static double DistanceBetween(double lat1, double long1, double lat2, double long2);
    static double CourseTo(double lat1, double long1, double lat2, double long2);
};
