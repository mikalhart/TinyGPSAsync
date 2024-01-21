#include "TinyGPSAsync.h"
#include "esp_task_wdt.h"

void TinyGPSAsync::TaskSpecific::processNewSentences()
{
    vector<ParsedSentence> ParsedSentences;

    for (auto s:NewSentences)
        ParsedSentences.push_back(ParsedSentence::FromString(s));

#if ARDUHAL_LOG_LEVEL >= ARDUHAL_LOG_LEVEL_DEBUG
    for (auto s:ParsedSentences)
        Serial.printf("[%s] ", s.SentenceId().c_str());
    Serial.println();
#endif
    log_d("Thread processing %d sentences", ParsedSentences.size());

    if (xSemaphoreTake(gpsMutex, portMAX_DELAY) == pdTRUE)
    {
        for (auto ps:ParsedSentences)
        {
            Counters.encodedCharCount += ps.CharCount();
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
                    hasNewSentence = true;
                }

                if (id == "RMC")
                    ++Counters.rmcCount;
                
                if (id == "GGA")
                    ++Counters.ggaCount;

                if (id == "GSV" && ps.ChecksumValid())
                {
                    uint16_t msgCount = (uint16_t)strtoul(ps[1].c_str(), NULL, 10);
                    uint16_t msgNo = (uint16_t)strtoul(ps[2].c_str(), NULL, 10);
                    uint16_t svsInView = (uint16_t)strtoul(ps[3].c_str(), NULL, 10);
                    log_d("GSV: count: %d no: %d in view: %d", msgCount, msgNo, svsInView);
                    for (int i=0; i<4 && 4 * (msgNo - 1) + i < svsInView; ++i)
                    {
                        TinyGPSAsync::SatelliteItem::SatInfo sat;
                        sat.prn = (uint16_t)strtoul(ps[4 + 4 * i].c_str(), NULL, 10);
                        sat.elevation = (uint16_t)strtoul(ps[5 + 4 * i].c_str(), NULL, 10);
                        sat.azimuth = (uint16_t)strtoul(ps[6 + 4 * i].c_str(), NULL, 10);
                        sat.snr = (uint16_t)strtoul(ps[7 + 4 * i].c_str(), NULL, 10);
                        SatelliteBuffer.push_back(sat);
                    }
                    
                    if (msgNo == msgCount)
                    {
                        AllSatellites = SatelliteBuffer;
                        SatelliteBuffer.clear();
                    }
                }
            }
            else
            {
                log_d("Invalid sentence received");
                ++Counters.invalidSentenceCount;
            }
        }
        NewSentences.clear();
        xSemaphoreGive(gpsMutex);
    }
}

void TinyGPSAsync::TaskSpecific::gpsTask(void *pvParameters)
{
    TinyGPSAsync::TaskSpecific *pThis = (TinyGPSAsync::TaskSpecific *)pvParameters;
    log_d("gpsTask started");

    char buf[200];
    int index = 0;

    while (pThis->taskActive)
    {
        while (pThis->stream->available() && pThis->NewSentences.size() <= 1)
        {
            char c = pThis->stream->read();
            ++pThis->Counters.encodedCharCount;
            if (c == '\r')
            {
                buf[index] = 0;
                pThis->NewSentences.push_back(buf);
                index = 0;
            }
            else if (c != '\n')
            {
                buf[index++] = c;
                if (index == sizeof(buf) - 1) // discard?
                    index = 0;
            }
        }
        delay(1); // to avoid killing Idle thread watchdog
        if (pThis->NewSentences.size() > 0)
            pThis->processNewSentences();
    }
    vTaskDelete(NULL);
}
