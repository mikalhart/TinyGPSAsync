#include <map>
using namespace std;

namespace TinyGPS
{
    struct Ubx
    {
        uint8_t sync[2];
        uint8_t clss;
        uint8_t id;
        vector<uint8_t> payload;
        uint8_t chksum[2];
    };

    class ParsedUbxPacket
    {
    private:
        Ubx ubx;
        bool valid = false;
        bool checksumValid = false;
        mutable bool isNew = false;
        uint32_t lastUpdateTime = 0;
        uint8_t charCount = 0;

        static void updateChecksum(byte &ck_A, byte &ck_B, byte b)
        {
            ck_A += b;
            ck_B += ck_A;
        }
        
    public:
        /// @brief Tests whether the NMEA checksum is correct
        /// @return true if Checksum is present and valid
        bool IsValid() const            { return valid; }
        bool ChecksumValid() const      { return checksumValid; }
        bool IsNew() const              { bool temp = isNew; isNew = false; return temp; }
        uint32_t Age() const            { return Utils::msticks() - lastUpdateTime; }
        uint32_t Timestamp() const      { return lastUpdateTime; }
        byte Class() const              { return ubx.clss; }
        byte Id() const                 { return ubx.id; }
        void Clear()                    { ubx.clss = ubx.id = 0; ubx.payload.clear(); }
        uint8_t CharCount() const       { return charCount; }
        const Ubx &Packet() const       { return ubx; }

        static ParsedUbxPacket FromUbx(const Ubx &ubx);
        string ToString() const;
        vector<uint8_t> ToBuffer() const;
    };

    struct UbxPackets
    {
        ParsedUbxPacket LastUbxPacket;
        std::map<string, ParsedUbxPacket> AllUbxPackets;
        const ParsedUbxPacket &operator[] (const char *id) const 
        { 
            static ParsedUbxPacket empty;
            auto ret = AllUbxPackets.find(id);
            return ret == AllUbxPackets.end() ? empty : ret->second;
        }
    };
}