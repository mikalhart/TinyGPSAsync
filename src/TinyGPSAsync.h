#include <Arduino.h>
#include <map>
#include <vector>
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
        string DiagnosticString();

        const Snapshot &GetSnapshot() { syncSnapshot(); return snapshot; }
        const Satellites &GetSatellites() { syncSatellites(); return satellites;}
        const Sentences &GetSentences() { syncSentences(); return sentences; }
        const Statistics &GetStatistics() { syncStatistics(); return statistics; }
        bool NewSnapshotAvailable() { return task.hasNewSnapshot; }
        bool NewSentenceAvailable() { return task.hasNewSentences; }
        bool NewCharactersAvailable() { return task.hasNewCharacters; }
        bool NewSatellitesAvailable() { return task.hasNewSatellites; }

    private:
        uint32_t startTime;
        TaskSpecific task;
        Snapshot snapshot;
        Satellites satellites;
        Sentences sentences;
        Statistics statistics;

        void syncStatistics();
        void syncSatellites();
        void syncSentences();
        void syncSnapshot();
        void processGGA(const ParsedSentence &sentence);
        void processRMC(const ParsedSentence &sentence);

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