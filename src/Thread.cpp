#include "TinyGPSAsync.h"
#include "esp_task_wdt.h"

void TaskSpecific::processGarbageCharacters(int count)
{
    if (xSemaphoreTake(gpsMutex, portMAX_DELAY) == pdTRUE)
    {
        Counters.encodedCharCount += count;
        hasNewCharacters = true;
        xSemaphoreGive(gpsMutex);
    }
}

void TaskSpecific::processNewSentence(string &s)
{
    ParsedSentence ps = ParsedSentence::FromString(s);

    if (xSemaphoreTake(gpsMutex, portMAX_DELAY) == pdTRUE)
    {
        Counters.encodedCharCount += s.length();
        hasNewCharacters = true;
        if (ps.IsValid())
        {
            ++Counters.validSentenceCount;
            if (ps.ChecksumValid())
                ++Counters.passedChecksumCount;
            else
                ++Counters.failedChecksumCount;
            string id = ps.SentenceId();
            if (id != "")
            {
                LastSentence = AllSentences[id] = ps;
                hasNewSentences = true;
            }

            if (id == "RMC")
            {
                ++Counters.rmcCount;
                hasNewSnapshot = true;
            }
            
            if (id == "GGA")
            {
                ++Counters.ggaCount;
                hasNewSnapshot = true;
            }

            if (id == "GSV" && ps.ChecksumValid())
            {
                uint16_t msgCount = (uint16_t)strtoul(ps[1].c_str(), NULL, 10);
                uint16_t msgNo = (uint16_t)strtoul(ps[2].c_str(), NULL, 10);
                uint16_t svsInView = (uint16_t)strtoul(ps[3].c_str(), NULL, 10);
                log_d("GSV: count: %d no: %d in view: %d", msgCount, msgNo, svsInView);
                for (int i = 0; i < 4 && 4 * (msgNo - 1) + i < svsInView; ++i)
                {
                    SatInfo sat;
                    sat.prn = (uint16_t)strtoul(ps[4 + 4 * i].c_str(), NULL, 10);
                    sat.elevation = (uint16_t)strtoul(ps[5 + 4 * i].c_str(), NULL, 10);
                    sat.azimuth = (uint16_t)strtoul(ps[6 + 4 * i].c_str(), NULL, 10);
                    sat.snr = (uint16_t)strtoul(ps[7 + 4 * i].c_str(), NULL, 10);
                    SatelliteBuffer.push_back(sat);
                }
                
                if (msgNo == msgCount)
                {
                    AllSatellites = SatelliteBuffer;
                    SatelliteTalkerId = ps.TalkerId();
                    SatelliteBuffer.clear();
                    hasNewSatellites = true;
                }
            }
        }
        else
        {
            log_d("Invalid sentence received");
            ++Counters.invalidSentenceCount;
        }
        xSemaphoreGive(gpsMutex);
    }
}

void TaskSpecific::gpsTask(void *pvParameters)
{
    TaskSpecific *pThis = (TaskSpecific *)pvParameters;
    log_d("gpsTask started");

    char buf[200];
    int index = 0;

    while (pThis->taskActive)
    {
        while (pThis->stream->available())
        {
            char c = pThis->stream->read();
            if (c == '\r')
            {
                buf[index] = 0;
                string s = buf;
                pThis->processNewSentence(s);
                index = 0;
            }
            else if (c != '\n')
            {
                buf[index++] = c;
                if (index == sizeof(buf) - 1) // discard?
                {
                    pThis->processGarbageCharacters(index);
                    index = 0;
                }
            }
        }
        delay(1); // to avoid killing Idle thread watchdog
    }
    vTaskDelete(NULL);
}
