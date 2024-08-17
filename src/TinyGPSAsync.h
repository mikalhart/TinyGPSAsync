#include <Arduino.h>
#include <map>
#include <vector>

#if !defined(ESP32)
#error "This library only works with ESP32"
#endif

#include "esp_task_wdt.h"
using namespace std;

#define _GPS_VERSION "0.0.2" // software version of this library

/*
    Item : IsNew, IsValid, Age(), 
    GetSnapshot();
        IsNew();
        Location.Lat.AsDouble() <- rawlat
        Quality
            IsNew(), Vale
        Fix
    gps.GetSnapshot().IsNew();
    gps.GetSnapshot().Location;



class TinyGPSAsync
{
private:
    struct Item
    {
    protected:
        uint32_t lastUpdateTime = 0;
        bool everUpdated = false;
        bool isNew = false;
    public:
        bool     IsNew()       { bool temp = isNew; isNew = false; return temp; }
        bool     EverUpdated() { return everUpdated; }
        uint32_t Age()         { return everUpdated ? millis() - lastUpdateTime : ULONG_MAX; }
        uint32_t Timestamp()   { return lastUpdateTime; }
    };

public:
    class Snapshot
    {
        static constexpr double _GPS_MPH_PER_KNOT = 1.15077945;
        static constexpr double _GPS_MPS_PER_KNOT = 0.51444444;
        static constexpr double _GPS_KMPH_PER_KNOT = 1.852;
        static constexpr double _GPS_MILES_PER_METER = 0.00062137112;
        static constexpr double _GPS_KM_PER_METER = 0.001;
        static constexpr double _GPS_FEET_PER_METER = 3.2808399;
        static constexpr double _GPS_EARTH_RADIUS = 6372795;

    public:
        struct LocationItem : public Item
        {
        public:
            struct Raw
            {
                struct RawD
                {
                    uint16_t deg = 0;
                    uint32_t billionths = 0;
                    bool negative = false;
                } Lat, Lng;
            };

            struct Dbl
            {
                double Lat, Lng;
            };
        private:
            Raw r;
            static void parseDegrees(const char *degTerm, const char *nsewTerm, Raw::RawD &deg);

        public:
            Raw AsRaw() { isNew = false; return r; }
            Dbl Get();
        };
        LocationItem Location;

        struct QualityItem : public Item
        {
        public:
            enum QualityEnum { Invalid = '0', GPS = '1', DGPS = '2', PPS = '3', RTK = '4', FloatRTK = '5', Estimated = '6', Manual = '7', Simulated = '8' };
        private:
            QualityEnum q = Invalid;
            void parse(const char *term) { q = (QualityEnum)*term; }
        public:
            QualityEnum Get() { isNew = false; return q; }
        };

        struct FixStatusItem : public Item
        {
        public:
            enum StatusEnum { N = 'N', V = 'V' , A = 'A'};
        private:
            StatusEnum s = N;
            void parse(const char *term) { s = (StatusEnum)*term; }
        public:
            StatusEnum Get() { isNew = false; return s; }
            bool IsPositionValid() { return s == A; }
        };

        struct DateItem : public Item
        {
        public:
            struct Date
            {
                uint16_t Year = 0;
                uint8_t Month = 0;
                uint8_t Day = 0;
            };
        private:
            uint32_t d = 0;
            void parse(const char *term) { d = atol(term); }
        public:
            Date Get();
        };

        struct DecimalItem : public Item
        {
        protected:
            int32_t val = 0;
            void parse(const char *term);
        };

        struct TimeItem : public DecimalItem
        {
        public:
            struct Time
            {
                uint8_t Hour = 0;
                uint8_t Minute = 0;
                uint8_t Second = 0;
                uint8_t Centisecond = 0;
            };
            Time Get();
        };

        struct SpeedItem : public DecimalItem
        {
            double GetAsKnots()    { isNew = false; return val / 100.0; }
            double GetAsMph()      { isNew = false; return _GPS_MPH_PER_KNOT * val / 100.0; }
            double GetAsMps()      { isNew = false; return _GPS_MPS_PER_KNOT * val / 100.0; }
            double GetAsKmph()     { isNew = false; return _GPS_KMPH_PER_KNOT * val / 100.0; }
            double Get()           { return GetAsKnots(); }
        };

        struct CourseItem : public DecimalItem
        {
            double Get()      { isNew = false; return val / 100.0; }
        };

        struct AltitudeItem : public DecimalItem
        {
            double GetAsMeters()       { isNew = false; return val / 100.0; }
            double GetAsMiles()        { isNew = false; return _GPS_MILES_PER_METER * val / 100.0; }
            double GetAsKilometers()   { isNew = false; return _GPS_KM_PER_METER * val / 100.0; }
            double GetAsFeet()         { isNew = false; return _GPS_FEET_PER_METER * val / 100.0; }
            double Get()               { return GetAsMeters(); }
        };

        struct IntegerItem : public Item
        {
        protected:
            uint32_t val = 0;
            void parse(const char *term) { val = atol(term); }
        };

        struct SatelliteCountItem : public IntegerItem
        {
            uint32_t Get()             { isNew = false; return val; }
        };

        struct HDOPItem : public DecimalItem
        {
            double Get() { isNew = false; return val / 100.0; }
        };
    public:
        LocationItem Location;
        DateItem Date;
        TimeItem Time;
        SpeedItem Speed;
        CourseItem Course;
        AltitudeItem Altitude;
        SatelliteCountItem SatelliteCount;
        QualityItem Quality;
        FixStatusItem FixStatus;
        HDOPItem HDOP;
    };
    private:
        Snapshot snapshot;

    public:
    class Satellites
    {
    public:
        struct SatInfo
        {
            uint16_t prn;
            uint8_t elevation;
            uint16_t azimuth;
            uint8_t snr;
        };
    private:
        struct SatelliteItem : public Item
        {
        private:
            vector<SatInfo> s;
        public:
            vector<SatInfo> Get() { isNew = false; return s; }
        };
    public:
        SatelliteItem Satellites;
    };

    class ParsedSentence
    {
    private:
        vector<string> fields;
        bool hasChecksum = false;
        bool checksumValid = false;
        bool isNew = false;
        uint32_t lastUpdateTime = 0;
        uint8_t charCount = 0;
        
    public:
        bool IsValid() const            { return fields.size() > 0  && fields[0].length() == 6 && fields[0][0] == '$' && hasChecksum; }
        bool ChecksumValid() const      { return checksumValid; }
        bool IsNew()                    { bool temp = isNew; isNew = false; return temp; }
        uint32_t Age() const            { return millis() - lastUpdateTime; }
        uint32_t Timestamp() const      { return lastUpdateTime; }
        string TalkerId() const         { return IsValid() ? fields[0].substr(1, 2) : ""; }
        string SentenceId() const       { return IsValid() ? fields[0].substr(3) : ""; }
        void Clear()                    { fields.clear(); }
        string operator[](int field)    { return field >= fields.size() ? "" : fields[field]; }
        uint8_t CharCount()             { return charCount; }

        static ParsedSentence FromString(const string &str);
        string Get();
    };

    class Sentences
    {
        struct SentenceItem : public Item
        {
            friend class TinyGPSAsync;
        private:
            ParsedSentence LastSentence;
            std::map<string, ParsedSentence> AllSentences;
        public:
            const std::map<string, ParsedSentence> &Get() { isNew = false; return AllSentences; }
            ParsedSentence &Get(const char *id)           { isNew = false; return AllSentences[id]; }
            ParsedSentence &operator[] (const char *id)   { return Get(id); }
        };
        SentenceItem Sentence;
    };

    struct StatisticsCounters
    {
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

    class Diagnostics
    {
        struct DiagnosticItem : public Item
        {
            friend TinyGPSAsync;
        private:
            StatisticsCounters Counters;

        public:
            enum {OK = 0, STREAM, WIRING, BAUD, OVERFLOW, MISSING };
            StatisticsCounters Get() { isNew = false; return Counters; }
            int Status();
            string StatusString(int status);
        };
        DiagnosticItem Diagnostic;
    };

    private:
        uint32_t startTime;

        class TaskSpecific
        {
        public:
            /* Items guarded by mutex */
            ParsedSentence LastSentence;
            std::map<string, ParsedSentence> AllSentences;
            StatisticsCounters Counters;
            vector<TinyGPSAsync::Satellites::SatInfo> AllSatellites;

            /* Internal items (not guarded) */
            volatile bool hasNewSentence = false;
            vector<TinyGPSAsync::Satellites::SatInfo> SatelliteBuffer;
            Stream *stream = nullptr;
            SemaphoreHandle_t gpsMutex = xSemaphoreCreateMutex();


            volatile bool taskActive = false;
            void processNewSentence(string &s);
            static void gpsTask(void *pvParameters);

            void Clear()
            {
                Counters.Clear();
                AllSentences.clear();
                AllSatellites.clear();
            }
        } task;

        void processGGA(ParsedSentence &sentence);
        void processRMC(ParsedSentence &sentence);
        void process();

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
