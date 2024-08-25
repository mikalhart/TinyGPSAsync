#include <atomic>
#include <map>
#include <vector>
#include <string>
using namespace std;

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

private:
    /* Internal items (not guarded) */
    vector<SatInfo> SatelliteBuffer;
    Stream *stream = nullptr;
public:
    void Clear()
    {
        Counters.Clear();
        AllSentences.clear();
        AllSatellites.clear();
        SatelliteTalkerId = "";
    }
    const Stream *GetStream() const { return stream; }
    SemaphoreHandle_t gpsMutex = xSemaphoreCreateMutex();
    void processNewSentence(string &s);
    void processGarbageCharacters(int count);
    static void gpsTask(void *pvParameters);
    void end()
    {
        if (taskActive)
        {
            taskActive = false;
            vTaskDelay(500);
            stream = nullptr;
        }
    }

    bool begin(Stream &str)
    {
        end();
        stream = &str;
        Clear();
        taskActive = true;

        xTaskCreatePinnedToCore(gpsTask, "gpsTask", 10000, this, /* tskIDLE_PRIORITY */ uxTaskPriorityGet(NULL), NULL, 0);

        return true;
    }
};
