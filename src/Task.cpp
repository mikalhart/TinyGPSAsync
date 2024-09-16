#include "TinyGPSAsync.h"
#include "esp_task_wdt.h"

void TaskSpecific::postNewUnknownPacket()
{
    if (xSemaphoreTake(gpsMutex, portMAX_DELAY) == pdTRUE)
    {
        Counters.encodedCharCount += buffer.size();
        hasNewCharacters = true;
        hasNewUnknownPacket = true;
        LastUnknownPacket = buffer;
        buffer.clear();

        xSemaphoreGive(gpsMutex);
    }
}

void TaskSpecific::handleUnknownBytes()
{
    unsigned long start = Utils::msticks();
    while (true)
    {
        if (stream->available())
        {
            byte c = stream->peek();
            if (c == '$' || c == 0xB5)
                break;
            buffer.push_back(stream->read());
            if (buffer.size() == buffer.capacity())
                postNewUnknownPacket();
        }
        if (Utils::msticks() - start > 100)
        {
            vTaskDelay(1);
            start = Utils::msticks();
        }
    }
    postNewUnknownPacket();
}

void TaskSpecific::postNewUbxPacket(const Ubx &ubx)
{
    ParsedUbxPacket pu = ParsedUbxPacket::FromUbx(ubx);
    if (xSemaphoreTake(gpsMutex, portMAX_DELAY) == pdTRUE)
    {
        Counters.encodedCharCount += 8 + ubx.payload.size();
        hasNewCharacters = true;
        if (pu.IsValid())
        {
            ++Counters.validUbxCount;
            if (pu.ChecksumValid())
            {
                ++Counters.passedChecksumCount;
                string id = std::to_string(ubx.clss) + "." + std::to_string(ubx.id);
                LastUbxPacket = NewUbxPackets[id] = pu;
                hasNewUbxPackets = true;
                if (id == "1.7")
                {
                    ++Counters.ubxNavPvtCount;
                    SnapshotUbxPackets[id] = pu;
                    hasNewSnapshot = true;
                }
                else if (id == "1.53")
                {
                    ++Counters.ubx153Count;
                }
            }
            else
            {
                ++Counters.failedChecksumCount;
            }
        }
        else
        {
            log_d("Invalid ubx received");
        }
        xSemaphoreGive(gpsMutex);
    }
}

void TaskSpecific::handleUbxPacket()
{
    unsigned long start = Utils::msticks();
    uint16_t payloadLen = 0;
    while (true)
    {
        if (stream->available())
        {
            byte b = stream->read();
            buffer.push_back(b);
            if (buffer.size() == buffer.capacity())
            {
                postNewUnknownPacket();
                break;
            }
            
            if (buffer.size() == 2 && b != 0x62)
                break;

            if (buffer.size() == 6)
            {
                payloadLen = 0x100 * (byte)buffer[5] + (byte)buffer[4];
                if (payloadLen >= buffer.capacity() - 8)
                {
                    postNewUnknownPacket();
                    break;
                }
            }
            
            if (buffer.size() == 6 + payloadLen + 2)
            {
                Ubx ubx;
                ubx.sync[0] = buffer[0];
                ubx.sync[1] = buffer[1];
                ubx.clss = buffer[2];
                ubx.id = buffer[3];
                ubx.payload = std::vector<uint8_t>(buffer.begin() + 6, buffer.begin() + 6 + payloadLen);
                ubx.chksum[0] = buffer[6 + payloadLen];
                ubx.chksum[1] = buffer[6 + payloadLen + 1];
                postNewUbxPacket(ubx);
                buffer.clear();
                break;
            }
        }
        if (Utils::msticks() - start > 100)
        {
            vTaskDelay(1);
            start = Utils::msticks();
        }
    }
}

void TaskSpecific::postNewSentence(const string &s)
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
            {
                ++Counters.passedChecksumCount;
                string id = ps.SentenceId();
                if (id != "")
                {
                    LastSentence = NewSentences[id] = ps;
                    hasNewSentences = true;
                }

                if (id == "RMC")
                {
                    ++Counters.rmcCount;
                    SnapshotSentences[id] = ps;
                    hasNewSnapshot = true;
                }
                
                if (id == "GGA")
                {
                    ++Counters.ggaCount;
                    SnapshotSentences[id] = ps;
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
                ++Counters.failedChecksumCount;
            }
        }
        else
        {
            ++Counters.invalidSentenceCount;
        }
        xSemaphoreGive(gpsMutex);
    }
}

void TaskSpecific::handleNMEASentence()
{
    unsigned long start = Utils::msticks();
    while (true)
    {
        if (stream->available())
        {
            char c = stream->read();
            buffer.push_back((uint8_t)c);
            if (c == '\n')
            {
                if (buffer.size() >= 2 && buffer[buffer.size() - 2] == (uint8_t)'\r')
                {
                    string nmea(buffer.begin(), buffer.end() - 2);
                    postNewSentence(nmea);
                    buffer.clear();
                    break;
                }
                postNewUnknownPacket();
                Serial.printf("*** Sentence error: no carriage return\n");
                break;
            }

            if (buffer.size() == buffer.capacity())
            {
                postNewUnknownPacket();
                break;
            }
        }
        if (Utils::msticks() - start > 100)
        {
            vTaskDelay(1);
            start = Utils::msticks();
        }
    }
}

void TaskSpecific::mainLoop(void *pvParameters)
{
    TaskSpecific *pThis = (TaskSpecific *)pvParameters;
    log_d("gpsTask started");
    pThis->buffer.reserve(2048);

    while (pThis->taskActive)
    {
        if (pThis->stream->available())
        {
            byte c = pThis->stream->peek();
            if (c == '$')
                pThis->handleNMEASentence();
            else if (c == 0xB5)
                pThis->handleUbxPacket();
            else
                pThis->handleUnknownBytes();
if (!pThis->buffer.empty())
Serial.printf("OOPS\n");
        }

        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}
