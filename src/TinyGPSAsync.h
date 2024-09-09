#include <Arduino.h>
#include <map>
#include "Utils.h"
#include "Snapshot.h"
#include "Task.h"

#if !defined(ESP32)
#error "This library only works with ESP32"
#endif

#include "esp_task_wdt.h"
using namespace std;
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

        const Snapshot &GetSnapshot()     { syncSnapshot(); return snapshot; }
        const Satellites &GetSatellites() { syncSatellites(); return satellites;}
        const Sentences &GetSentences()   { syncSentences(); return sentences; }
        const UbxPackets &GetUbxPackets() { syncUbxPackets(); return ubxPackets; }
        const Statistics &GetStatistics() { syncStatistics(); return statistics; }
        const vector<uint8_t> &GetUnknownPacket() { syncUnknownPackets(); return lastUnknownPacket; }
        bool NewSnapshotAvailable()       { return task.hasNewSnapshot; }
        bool NewSatellitesAvailable()     { return task.hasNewSatellites; }
        bool NewSentenceAvailable()       { return task.hasNewSentences; }
        bool NewUbxPacketAvailable()      { return task.hasNewUbxPackets; }
        bool NewCharactersAvailable()     { return task.hasNewCharacters; }
        bool NewUnknownPacketAvailable()  { return task.hasNewUnknownPacket; }

    private:
        uint32_t startTime;
        TaskSpecific task;
        Snapshot snapshot;
        Satellites satellites;
        Sentences sentences;
        UbxPackets ubxPackets;
        Statistics statistics;
        vector<uint8_t> lastUnknownPacket;

        void syncStatistics();
        void syncSatellites();
        void syncSentences();
        void syncUbxPackets();
        void syncSnapshot();
        void syncUnknownPackets();
        void processGGA(const ParsedSentence &sentence);
        void processRMC(const ParsedSentence &sentence);
        void processUbxNavPvt(const ParsedUbxPacket &pu);
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