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
        std::atomic<bool> hasNewSatellites;
        std::atomic<bool> hasNewPacket;
            std::atomic<bool> hasNewUbxPackets;
            std::atomic<bool> hasNewUnknownPacket;
            std::atomic<bool> hasNewSentences;
        std::atomic<bool> hasNewSnapshot;
        std::atomic<bool> taskActive;

        /* Items guarded by mutex */
        NmeaPacket LastSentence;
        std::map<string, NmeaPacket> NewSentences;
        std::map<string, NmeaPacket> SnapshotSentences;

        UbxPacket LastUbxPacket;
        std::map<string, UbxPacket> NewUbxPackets;
        std::map<string, UbxPacket> SnapshotUbxPackets;

        UnknownPacket LastUnknownPacket;

        enum {UNKNOWN, NMEA, UBX} lastPacketType = UNKNOWN;

        Statistics Counters;
        std::vector<SatelliteInfo> AllSatellites;
        std::string SatelliteTalkerId;

    private:
        std::vector<uint8_t> buffer;

        /* Internal items (not guarded) */
        std::vector<SatelliteInfo> SatelliteStaging;
        Stream *stream = nullptr;
        void clear()
        {
            Counters.clear();
            NewSentences.clear();
            AllSatellites.clear();
            SatelliteTalkerId = "";
            hasNewPacket = hasNewSatellites = hasNewSentences = hasNewSnapshot = false;
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