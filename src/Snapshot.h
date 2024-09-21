#include "Satellites.h"
#include "Packet.h"
#include "Ubx.h"
#include "Statistics.h"

namespace TinyGPS
{
    class Snapshot
    {
        friend class TinyGPSAsync;
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
            friend class TinyGPSAsync;
        protected:
            mutable uint32_t lastUpdateTime = 0;
            mutable bool everUpdated = false;
            mutable bool isNew = false;
            mutable bool isVoid = true;
            void stampIt(uint32_t timestamp) { everUpdated = isNew = true; lastUpdateTime = timestamp; }

        public:
            bool     IsNew() const       { return isNew; }
            bool     IsVoid() const      { return isVoid; }
            bool     EverUpdated() const { return everUpdated; }
            uint32_t Age() const         { return everUpdated ? Utils::msticks() - lastUpdateTime : ULONG_MAX; }
            uint32_t Timestamp() const   { return lastUpdateTime; }
        };

    public:
        struct LocationItem : public Item
        {
            friend class TinyGPSAsync;
        private:
            struct LocationAngle
            {
                uint16_t deg = 0;
                uint32_t billionths = 0;
                bool negative = false;
                void parseDegrees(const char *degTerm, const char *nsewTerm);
            } RawLat, RawLng;
            void parse(const string &termLat, const string &NS, const string &termLng, const string &EW, uint32_t timestamp)
            {
                stampIt(timestamp);
                isVoid = termLat.empty() || termLng.empty();
                if (!isVoid)
                {
                    RawLat.parseDegrees(termLat.c_str(), NS.c_str());
                    RawLng.parseDegrees(termLng.c_str(), EW.c_str());
                }
            }
            void set(int32_t lat, int32_t lng, uint32_t timestamp)
            {
                stampIt(timestamp);
                isVoid = false;
                if (RawLat.negative = lat < 0) lat = -lat;
                if (RawLng.negative = lng < 0) lng = -lng;
                RawLat.billionths = 100 * (lat % 10000000);
                RawLng.billionths = 100 * (lng % 10000000);
                RawLat.deg = lat / 10000000;
                RawLng.deg = lng / 10000000;
            }
            void clear(uint32_t timestamp)
            {
                stampIt(timestamp);
                isVoid = true;
            }
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
            friend class TinyGPSAsync;
        public:
            enum QualityEnum { Invalid = '0', GPS = '1', DGPS = '2', PPS = '3', RTK = '4', FloatRTK = '5', Estimated = '6', Manual = '7', Simulated = '8' };
            QualityEnum Value() const { isNew = false; return v; }
        private:
            QualityEnum v = Invalid;
            void parse(const string &term, uint32_t timestamp) 
            { 
                stampIt(timestamp);
                isVoid = term.empty();
                if (!isVoid)
                    v = (QualityEnum)term[0]; 
            }
        };

        struct FixStatusItem : public Item
        {
            friend class TinyGPSAsync;
        public:
            enum StatusEnum { N = 'N', V = 'V' , A = 'A'};
            StatusEnum Value() const { isNew = false; return isVoid ? N : v; }
            bool IsPositionValid() const { isNew = false; return v == A; }
        private:
            StatusEnum v = N;
            void parse(const string &term, uint32_t timestamp)
            {
                stampIt(timestamp);
                isVoid = term.empty();
                if (!isVoid)
                    v = (StatusEnum)term[0];
            }
            void set(StatusEnum fix, uint32_t timestamp)
            {
                stampIt(timestamp);
                isVoid = false;
                v = fix;
            }
        };

        struct IntegerItem : public Item
        {
            friend class TinyGPSAsync;
        protected:
            uint32_t v = 0;
            void parse(const string &term, uint32_t timestamp) 
            {
                stampIt(timestamp);
                isVoid = term.empty();
                if (!isVoid)
                    v = atol(term.c_str());
            }
        public:
            uint32_t Value() const { isNew = false; return v; }
        };

        struct DecimalItem : public Item
        {
            friend class TinyGPSAsync;
        protected:
            int32_t v = 0;
            void parse(const string &term, uint32_t timestamp);
        public:
            int32_t Value() const { isNew = false; return v; }
        };

        struct DateItem : public Item
        {
            uint16_t Year() const { isNew = false; return m_year; }
            uint8_t Month() const { isNew = false; return m_month; }
            uint8_t Day() const   { isNew = false; return m_day; }

            friend class TinyGPSAsync;
        private:
            uint16_t m_year;
            uint8_t m_month;
            uint8_t m_day;
            
            void set(uint16_t year, uint8_t month, uint8_t day, uint32_t timestamp) 
            { 
                stampIt(timestamp);
                isVoid = false;
                m_year = year;
                m_month = month;
                m_day = day;
            }

            void clear(uint32_t timestamp) 
            { 
                stampIt(timestamp);
                isVoid = true;
            }

            void parse(const string &term, uint32_t timestamp) 
            {
                stampIt(timestamp);
                isVoid = term.empty();
                if (!isVoid)
                {
                    uint32_t v = atol(term.c_str());
                    m_year = 2000 + v % 100;
                    m_month = (v / 100) % 100;
                    m_day = v / 10000;
                }
            }
        };

        struct TimeItem : public DecimalItem
        {
            uint8_t Hour() const        { isNew = false; return m_hour; }
            uint8_t Minute() const      { isNew = false; return m_minute; }
            uint8_t Second() const      { isNew = false; return m_second; }
            int32_t Nanos() const       { isNew = false; return m_nanos; }

            friend class TinyGPSAsync;
        private:
            uint8_t m_hour;
            uint8_t m_minute;
            uint8_t m_second;
            int32_t m_nanos;
            
            void set(uint8_t hour, uint8_t minute, uint8_t second, int32_t nanos, uint32_t timestamp) 
            { 
                stampIt(timestamp);
                isVoid = false;
                m_hour = hour;
                m_minute = minute;
                m_second = second;
                m_nanos = nanos;
            }

            void clear(uint32_t timestamp) 
            { 
                stampIt(timestamp);
                isVoid = true;
            }

            void parse(const string &term, uint32_t timestamp) 
            {
                DecimalItem::parse(term, timestamp);
                if (!isVoid)
                {
                    m_hour = v / 1000000;
                    m_minute = (v / 10000) % 100;
                    m_second = (v / 100) % 100;
                    m_nanos = 10000000 * (v % 100);
                }
            }
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
            double Radians() const  { isNew = false; return v * PI / 18000.0; }
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
}