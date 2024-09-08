#include "TinyGPSAsync.h"
#include "esp_task_wdt.h"

void TaskSpecific::flushBuffer()
{
    if (!buffer.empty() && xSemaphoreTake(gpsMutex, portMAX_DELAY) == pdTRUE)
    {
        Counters.encodedCharCount += buffer.length();
        buffer.clear();
        hasNewCharacters = true;
        xSemaphoreGive(gpsMutex);
    }
}

string msg;
void TaskSpecific::discardCharacters()
{
    unsigned long start = millis();
    flushBuffer();
    while (true)
    {
        if (stream->available())
        {
            byte c = stream->peek();
            if (c == '$' || c == 0xB5)
                break;
            buffer += stream->read();
            if (buffer.length() == buffer.capacity())
                flushBuffer();
        }
        if (millis() - start > 100)
        {
            vTaskDelay(1);
            start = millis();
        }
    }
char buf[100];
sprintf(buf, "DISC %d", buffer.size());
msg = buf;
    flushBuffer();
}

void TaskSpecific::processNewUbxPacket(const Ubx &ubx)
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

void TaskSpecific::processNewSentence(const string &s)
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

void TaskSpecific::tryParseSentence()
{
    unsigned long start = millis();
    flushBuffer();
    while (true)
    {
        if (stream->available())
        {
            char c = stream->read();
            buffer += c;
            if (c == '\n')
            {
                if (buffer.length() >= 2 && buffer[buffer.length() - 2] == '\r')
                {
                    string nmea = buffer.substr(0, buffer.length() - 2);
                    processNewSentence(nmea);
char buf[100];
sprintf(buf, "SENT %s %d", nmea.substr(0, 40).c_str(), buffer.size());
msg = buf;

                    buffer.clear();
                    break;
                }
                Serial.printf("*** Sentence error: no carriage return\n");
                flushBuffer();
                break;
            }
            if (buffer.length() == buffer.capacity())
            {
                Serial.printf("*** Sentence Error len: %s ***\n", buffer.c_str());
                flushBuffer();
                break;
            }
        }
        if (millis() - start > 100)
        {
            vTaskDelay(1);
            start = millis();
        }
    }
}

void TaskSpecific::tryParseUbxPacket()
{
    unsigned long start = millis();
    flushBuffer();
    uint16_t payloadLen = 0;
    while (true)
    {
        if (stream->available())
        {
            byte b = stream->read();
            buffer += b;
            bool fail = buffer.length() == buffer.capacity() || (buffer.length() == 2 && b != 0x62);
            if (fail)
            {
                flushBuffer();
                break;
            }

            if (buffer.length() == 6)
            {
                payloadLen = 0x100 * (byte)buffer[5] + (byte)buffer[4];
                if (payloadLen > 10000)
                {
                    flushBuffer();
                    break;
                }
            }
            
            if (buffer.length() == 6 + payloadLen + 2)
            {
                Ubx ubx;
                ubx.sync[0] = buffer[0];
                ubx.sync[1] = buffer[1];
                ubx.clss = buffer[2];
                ubx.id = buffer[3];
                ubx.payload = std::vector<uint8_t>(buffer.begin() + 6, buffer.begin() + 6 + payloadLen);
                ubx.chksum[0] = buffer[6 + payloadLen];
                ubx.chksum[1] = buffer[6 + payloadLen + 1];
                processNewUbxPacket(ubx);
char buf[100];
sprintf(buf, "UBX  (%x:%x) %d", ubx.clss, ubx.id, buffer.size());
msg = buf;
                buffer.clear();
                break;
            }
        }
        if (millis() - start > 100)
        {
            vTaskDelay(1);
            start = millis();
        }
    }
}

void TaskSpecific::parseStream(void *pvParameters)
{
    TaskSpecific *pThis = (TaskSpecific *)pvParameters;
    log_d("gpsTask started");
    pThis->buffer.reserve(10000);

    while (pThis->taskActive)
    {
unsigned long start = millis();
int availstart = pThis->stream->available();
        if (pThis->stream->available())
        {
            byte c = pThis->stream->peek();
            if (c == '$')
                pThis->tryParseSentence();
            else if (c == 0xB5)
                pThis->tryParseUbxPacket();
            else
                pThis->discardCharacters();
//if (isprint(c))
//   Serial.printf(" %c: %10s T(ms): %3lu   availstart: %5lu\n", c, msg.c_str(), (unsigned)(millis() - start), availstart, pThis->stream->available() - availstart);
//else
//   Serial.printf("%02X: %10s T(ms): %3lu   availstart: %5lu\n", c, msg.c_str(), (unsigned)(millis() - start), availstart, pThis->stream->available() - availstart);
        }

        vTaskDelay(1);
    }
    vTaskDelete(NULL);
}
