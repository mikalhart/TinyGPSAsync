#include <Arduino.h>
#include "Packet.h"
namespace TinyGPS
{
    std::string NmeaPacket::ToString() const
    {
        std::string str;
        for (int i = 0; i < fields.size(); ++i)
        {
            str += fields[i];
            if (i != fields.size() - 1)
                str += i == fields.size() - 2 ? '*' : ',';
        }
        return str;
    }

    /* static */
    NmeaPacket NmeaPacket::FromString(const std::string &str)
    {
        NmeaPacket s;
        s.lastUpdateTime = Utils::msticks();
        s.isNew = true;
        s.charCount = str.length();
        s.type = NMEA;
        size_t start = 0;
        size_t delimiterPos = str.find_first_of(",*");
        unsigned long calculatedChksum = 0;
        log_d("Parsing sentence %s", str.c_str());
        while (true)
        {
            std::string token = delimiterPos != std::string::npos ? str.substr(start, delimiterPos - start) : str.substr(start);
            s.fields.push_back(token);
            for (auto c : token.substr(s.fields.size() == 1 && s.fields[0].substr(0, 1) == "$" ? 1 : 0))
                calculatedChksum ^= c;

            if (delimiterPos == std::string::npos)
            {
                s.hasChecksum = s.checksumValid = false;
                log_d("Doesn't have any * checksum");
                break;
            }

            if (str[delimiterPos] == '*')
            {
                token = str.substr(delimiterPos + 1);
                s.fields.push_back(token);
                if (token.find_first_not_of("0123456789ABCDEFabcdef") != std::string::npos)
                {
                    s.hasChecksum = s.checksumValid = false;
                    log_e("Bad format checksum: '%s'", token.c_str());
                    log_e("Sentence was '%s'", str.c_str());
                }
                else
                {
                    unsigned long suppliedChecksum = strtoul(token.c_str(), NULL, 16);
                    s.hasChecksum = true;
                    s.checksumValid = suppliedChecksum == calculatedChksum;
                }
                break;
            }

            calculatedChksum ^= ',';
            start = delimiterPos + 1;
            delimiterPos = str.find_first_of(",*", start);
        }

        return s;
    }

    std::vector<uint8_t> NmeaPacket::ToBuffer() const
    {
        std::string str = ToString();
        return std::vector<uint8_t>(str.begin(), str.end());
    }

    std::string UbxPacket::ToString() const
    {
        char buf[100];
        sprintf(buf, "UBX: Class=%x Id=%x Len=%d Chksum=%x.%x (%s)", ubx.clss, ubx.id, ubx.payload.size(), ubx.chksum[0], ubx.chksum[1], checksumValid ? "valid" : "invalid");
        return buf;
    }

    UbxPacket UbxPacket::FromUbx(const Ubx &ubx)
    {
        UbxPacket u;
        uint16_t len = ubx.payload.size();
        u.lastUpdateTime = Utils::msticks();
        u.isNew = true;
        u.charCount = 8 + len;
        u.type = UBX;
        unsigned long calculatedChksum = 0;
        log_d("Parsing Ubx %d.%d", ubx.clss, ubx.id);
        u.valid = ubx.sync[0] == 0xB5 && ubx.sync[1] == 0x62;
        byte ck_A = 0, ck_B = 0;
        updateChecksum(ck_A, ck_B, ubx.clss);
        updateChecksum(ck_A, ck_B, ubx.id);
        updateChecksum(ck_A, ck_B, (byte)(len & 0xFF));
        updateChecksum(ck_A, ck_B, (byte)((len >> 8) & 0xFF));
        for (int i=0; i<len; ++i)
            updateChecksum(ck_A, ck_B, ubx.payload[i]);
        u.checksumValid = ck_A == ubx.chksum[0] && ck_B == ubx.chksum[1];
        u.ubx = ubx;
        return u;
    }

    std::vector<uint8_t> UbxPacket::ToBuffer() const
    {
        std::vector<uint8_t> ret;
        ret.push_back(ubx.sync[0]);
        ret.push_back(ubx.sync[1]);
        ret.push_back(ubx.clss);
        ret.push_back(ubx.id);
        ret.push_back(ubx.payload.size() & 0xFF);
        ret.push_back((ubx.payload.size() >> 8) & 0xFF);
        ret.insert(ret.end(), ubx.payload.begin(), ubx.payload.end());
        ret.push_back(ubx.chksum[0]);
        ret.push_back(ubx.chksum[1]);
        return ret;
    }

    UnknownPacket UnknownPacket::Empty;
    NmeaPacket NmeaPacket::Empty;
    UbxPacket UbxPacket::Empty;
}
