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
