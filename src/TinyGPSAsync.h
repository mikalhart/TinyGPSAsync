#include <Arduino.h>
#include <map>
#include <vector>
#include "Snapshot.h"
#include "Task.h"

#if !defined(ESP32)
#error "This library only works with ESP32"
#endif

#include "esp_task_wdt.h"
using namespace std;

#define _GPS_VERSION "0.0.3" // software version of this library

class TinyGPSAsync
{
public:
    enum Status {OK = 0, STREAM, WIRING, BAUD, OVERFLOW, MISSING };
    Status DiagnosticCode();
    string DiagnosticString();
    Snapshot snapshot;

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
    TaskSpecific task;

    void sync();
    void processGGA(ParsedSentence &sentence);
    void processRMC(ParsedSentence &sentence);

public:
    void end()
    {
        task.end();
    }

    bool begin(Stream &stream)
    {
        startTime = millis();
        return task.begin(stream);
    }

    static const char *Cardinal(double course);
    static double DistanceBetween(double lat1, double long1, double lat2, double long2);
    static double CourseTo(double lat1, double long1, double lat2, double long2);
};
