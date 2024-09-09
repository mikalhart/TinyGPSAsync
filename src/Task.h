#include <atomic>
#include <map>
#include <vector>
#include <string>
using namespace std;

namespace TinyGPS
{
    class TaskSpecific
    {
    public:
        /* For signaling to main thread */
        std::atomic<bool> hasNewCharacters;
        std::atomic<bool> hasNewSatellites;
        std::atomic<bool> hasNewSentences;
        std::atomic<bool> hasNewUbxPackets;
        std::atomic<bool> hasNewUnknownPacket;
        std::atomic<bool> hasNewSnapshot;
        std::atomic<bool> taskActive;

        /* Items guarded by mutex */
        ParsedSentence LastSentence;
        std::map<string, ParsedSentence> NewSentences;
        std::map<string, ParsedSentence> SnapshotSentences;

        ParsedUbxPacket LastUbxPacket;
        std::map<string, ParsedUbxPacket> NewUbxPackets;
        std::map<string, ParsedUbxPacket> SnapshotUbxPackets;

        std::vector<uint8_t> LastUnknownPacket;

        Statistics Counters;
        vector<SatInfo> AllSatellites;
        string SatelliteTalkerId;

    private:
        vector<uint8_t> buffer;

        /* Internal items (not guarded) */
        vector<SatInfo> SatelliteBuffer;
        Stream *stream = nullptr;
        void clear()
        {
            Counters.clear();
            NewSentences.clear();
            AllSatellites.clear();
            SatelliteTalkerId = "";
            hasNewCharacters = hasNewSatellites = hasNewSentences = hasNewSnapshot = false;
        }
        void postNewSentence(const string &s);
        void postNewUbxPacket(const Ubx &ubx);
        void postNewUnknownPacket();
        void handleNMEASentence();
        void handleUbxPacket();
        void handleUnknownBytes();
        static void mainLoop(void *pvParameters);

    public:
        SemaphoreHandle_t gpsMutex = xSemaphoreCreateMutex();
        const Stream *GetStream() const { return stream; }
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
            clear();
            taskActive = true;

            xTaskCreatePinnedToCore(mainLoop, "GPS Parsing Task", 10000, this, /* tskIDLE_PRIORITY */ uxTaskPriorityGet(NULL), NULL, 0);

            return true;
        }
    };
};