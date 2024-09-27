#include <Arduino.h>
#include <map>
//#include "Utils.h"
#include "Snapshot.h"
#include "Task.h"

#if !defined(ESP32)
#error "This library only works with ESP32"
#endif

#include "esp_task_wdt.h"
using namespace TinyGPS;
namespace TinyGPS
{
    #define _GPS_VERSION "0.0.6" // software version of this library

    class TinyGPSAsync
    {
    public:
        /// @brief Diagnostic Code
        enum Status {OK = 0, STREAM, WIRING, BAUD, OVERFLOW, MISSING };
        Status DiagnosticCode();
        const char *DiagnosticString();

        const Snapshot &GetSnapshot()                { syncSnapshot(); task.hasNewSnapshot = false; return snapshot; }
        const vector<SatelliteInfo> &GetSatellites() { syncSatellites(); task.hasNewSatellites = false; return satellites;}
        const Packet &GetLatestPacket()              { syncPackets(); task.hasNewPacket = false; return *latest; }
        const SentenceMap &GetSentenceMap()          { syncPackets(); task.hasNewSentences = false; return sentenceMap; }
        const UbxPacketMap &GetUbxPacketMap()        { syncPackets(); task.hasNewUbxPackets = false; return ubxPacketMap; }
        const Statistics &GetStatistics()            { syncStatistics(); return statistics; }
        bool NewSnapshotAvailable()                  { return task.hasNewSnapshot; }
        bool NewSatellitesAvailable()                { return task.hasNewSatellites; }
        bool NewSentenceAvailable()                  { return task.hasNewSentences; }
        bool NewUbxPacketAvailable()                 { return task.hasNewUbxPackets; }
        bool NewUnknownPacketAvailable()             { return task.hasNewUnknownPacket; }
        bool NewPacketAvailable()                    { return task.hasNewPacket; }


    private:
        uint32_t startTime;
        TaskSpecific task;
        Snapshot snapshot;
        vector<SatelliteInfo> satellites;
        SentenceMap sentenceMap;
        UbxPacketMap ubxPacketMap;
        Statistics statistics;
        NmeaPacket latestNmea;
        UbxPacket latestUbx;
        UnknownPacket latestUnknown;
        Packet *latest = &UnknownPacket::Empty;

        void syncStatistics();
        void syncSatellites();
        void syncPackets();
        void syncSnapshot();
        void processGGA(const NmeaPacket &sentence);
        void processRMC(const NmeaPacket &sentence);
        void processUbxNavPvt(const UbxPacket &pu);
        uint16_t makeU16(uint8_t lo, uint8_t hi) { return lo + (hi << 8); }
        int32_t makeI32(uint8_t lo0, uint8_t lo1, uint8_t lo2, uint8_t hi) { return lo0 + (lo1 << 8) + (lo2 << 16) + (hi << 24); }

    public:
        void end()
        {
            task.end();
        }

        bool begin(Stream &stream)
        {
            startTime = Utils::msticks();
            return task.begin(stream);
        }

        #include <chrono>

        static const char *Cardinal(double course);
        static double DistanceBetween(double lat1, double long1, double lat2, double long2);
        static double CourseTo(double lat1, double long1, double lat2, double long2);
    };
};