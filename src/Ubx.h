#include <map>
using namespace std;

namespace TinyGPS
{
    struct UbxPacketMap
    {
        std::map<string, UbxPacket> AllUbxPackets;
        const UbxPacket &operator[] (const char *id) const 
        { 
            auto ret = AllUbxPackets.find(id);
            return ret == AllUbxPackets.end() ? UbxPacket::Empty : ret->second;
        }
    };

    class UBXProtocol
    {
        static void updateChecksum(byte &ck_A, byte &ck_B, byte b)
        {
            ck_A += b;
            ck_B += ck_A;
        }

        Stream &stream;

    public:
        UBXProtocol(Stream &stream) : stream(stream) {}

        static constexpr uint32_t PACKET_NMEA_GGA = 0x209100BB;
        static constexpr uint32_t PACKET_NMEA_GSV = 0x209100C5;
        static constexpr uint32_t PACKET_NMEA_GSA = 0x209100C0;
        static constexpr uint32_t PACKET_NMEA_VTG = 0x209100B1;
        static constexpr uint32_t PACKET_NMEA_RMC = 0x209100AC;
        static constexpr uint32_t PACKET_NMEA_GLL = 0x209100CA;
        static constexpr uint32_t PACKET_UBX_PVT  = 0x20910007;
        static constexpr uint32_t PACKET_UBX_SAT  = 0x20910016;

        static constexpr uint16_t RESET_HOT  = 0x0000;
        static constexpr uint16_t RESET_WARM = 0x0001;
        static constexpr uint16_t RESET_COLD = 0xFFFF;

        void sendpacket(uint8_t clss, uint8_t id, vector<uint8_t> &payload)
        {
            byte ck_A = 0, ck_B = 0;

            // calculate checksum
            updateChecksum(ck_A, ck_B, clss);
            updateChecksum(ck_A, ck_B, id);
            updateChecksum(ck_A, ck_B, (byte)(payload.size() & 0xFF));
            updateChecksum(ck_A, ck_B, (byte)((payload.size() >> 8) & 0xFF));
            for (int i=0; i<payload.size(); ++i)
                updateChecksum(ck_A, ck_B, payload[i]);

            // write packet
            stream.write(0xB5); stream.write(0x62);
            stream.write(clss); stream.write(id);
            stream.write(payload.size() & 0xFF);
            stream.write((payload.size() >> 8) & 0xFF);
            for (auto ch: payload)
                stream.write(ch);

            stream.write(ck_A);
            stream.write(ck_B);
        }

        void setpacketrate(uint32_t packet_type, uint8_t rate)
        {
            vector<uint8_t> payload = {0x01, 0x01, 0x00, 0x00};
            payload.reserve(9);
            payload.push_back((uint8_t)((packet_type >> 0) & 0xFF));
            payload.push_back((uint8_t)((packet_type >> 8) & 0xFF));
            payload.push_back((uint8_t)((packet_type >> 16) & 0xFF));
            payload.push_back((uint8_t)((packet_type >> 24) & 0xFF));
            payload.push_back(rate);
            sendpacket(0x06, 0x8A, payload);
            delay(100);
        }

        void setHZ(uint16_t ms)
        {
            uint32_t cfg_rate_meas = 0x30210001;
            uint32_t cfg_rate_nav  = 0x30210002;
            vector<uint8_t> payload = {0x01, 0x01, 0x00, 0x00};
            payload.reserve(10);
            payload.push_back((uint8_t)((cfg_rate_meas >> 0) & 0xFF));
            payload.push_back((uint8_t)((cfg_rate_meas >> 8) & 0xFF));
            payload.push_back((uint8_t)((cfg_rate_meas >> 16) & 0xFF));
            payload.push_back((uint8_t)((cfg_rate_meas >> 24) & 0xFF));
            payload.push_back((uint8_t)((ms >> 0) & 0xFF));
            payload.push_back((uint8_t)((ms >> 8) & 0xFF));
            sendpacket(0x06, 0x8A, payload);

            payload = {0x01, 0x01, 0x00, 0x00};
            payload.push_back((uint8_t)((cfg_rate_nav >> 0) & 0xFF));
            payload.push_back((uint8_t)((cfg_rate_nav >> 8) & 0xFF));
            payload.push_back((uint8_t)((cfg_rate_nav >> 16) & 0xFF));
            payload.push_back((uint8_t)((cfg_rate_nav >> 24) & 0xFF));
            payload.push_back((uint8_t)((1 >> 0) & 0xFF));
            payload.push_back((uint8_t)((1 >> 8) & 0xFF));
            sendpacket(0x06, 0x8A, payload);
        }

        void reset(uint16_t type)
        {
            vector<uint8_t> payload;
            payload.reserve(4);
            payload.push_back((uint8_t)((type >> 0) & 0xFF));
            payload.push_back((uint8_t)((type >> 8) & 0xFF));
            payload.push_back(2); // controlled software reset (GNSS)
            payload.push_back(0); // reserved0
            sendpacket(0x06, 0x04, payload);
            delay(500); // wait for reset to occur
        }
    };
}