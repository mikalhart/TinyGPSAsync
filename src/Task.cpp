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

void TaskSpecific::discardCharacter(byte c)
{
    buffer += c;
    if (buffer.length() == buffer.capacity())
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

//Serial.printf("-UBX %s\n", id.c_str());
                if (id == "1.7")
                {
                }
            }
            else
            {
Serial.print("-UBX failed checksum***\n");
                ++Counters.failedChecksumCount;
            }
        }
        else
        {
Serial.print("-UBX fail validity***\n");
            log_d("Invalid ubx received");
            ++Counters.invalidSentenceCount;
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
//Serial.printf("-Sentence %s\n", id.c_str());
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
Serial.print("Sent fail check***\n");
                ++Counters.failedChecksumCount;
            }
        }
        else
        {
Serial.print("Sent Inv ***\n");
            log_d("Invalid sentence received");
            ++Counters.invalidSentenceCount;
        }
        xSemaphoreGive(gpsMutex);
    }
}

void TaskSpecific::tryParseSentence()
{
    log_d("TryParseSentence");
//Serial.print("-Sentence-");
    flushBuffer();
    buffer += '$';
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
                    buffer.clear();
                }

                break;
            }
            if (buffer.length() == buffer.capacity())
            {
Serial.printf("Sentence Error len: %s ***\n", buffer.c_str());
                flushBuffer();
                break;
            }
        }
        vTaskDelay(0);  // to avoid killing Idle thread watchdog
    }
}

void TaskSpecific::tryParseUbxPacket()
{
    log_d("TryParseUbxPacket");
//Serial.print("-Packet-");
    flushBuffer();
    buffer += 0xB5;
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
    Serial.printf("\n\n***Packet error len: %d cap: %d\n b: %x buf: [%x,%x]\n\n", buffer.length(), buffer.capacity(), b, buffer[0], buffer[1]);
                flushBuffer();
                break;
            }

            if (buffer.length() == 6)
            {
                payloadLen = 0x100 * (byte)buffer[5] + (byte)buffer[4];
            }

            if (buffer.length() == 6 + payloadLen + 2)
            {
                Ubx ubx;
                ubx.sync[0] = buffer[0];
                ubx.sync[1] = buffer[1];
                ubx.clss = buffer[2];
                ubx.id = buffer[3];
                ubx.payload = vector<byte>(buffer.begin() + 6, buffer.begin() + 6 + payloadLen);
                ubx.chksum[0] = buffer[6 + payloadLen];
                ubx.chksum[1] = buffer[6 + payloadLen + 1];
                processNewUbxPacket(ubx);
                flushBuffer();
                break;
            }
        }
        vTaskDelay(0);  // to avoid killing Idle thread watchdog
    }
}

void TaskSpecific::parseStream(void *pvParameters)
{
    TaskSpecific *pThis = (TaskSpecific *)pvParameters;
    log_d("gpsTask started");
    pThis->buffer.reserve(20000);

    while (pThis->taskActive)
    {
        while (pThis->stream->available())
        {
            byte c = pThis->stream->read();
            if (c == '$')
                pThis->tryParseSentence();
            else if (c == 0xB5)
                pThis->tryParseUbxPacket();
            else
                pThis->discardCharacter(c);
        }
        vTaskDelay(0);  // to avoid killing Idle thread watchdog
    }
    vTaskDelete(NULL);
}
