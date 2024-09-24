#include <vector>
#include <map>
#include <string>
#include "Utils.h"
// using namespace std;

namespace TinyGPS
{
    class Packet
    {
        class NmeaPacket;
        class UbxPacket;
    protected:
        mutable bool isNew = false;
        uint32_t lastUpdateTime = 0;
        enum {UNKNOWN=0, NMEA=1, UBX=2} type = UNKNOWN;
    public:
        bool IsNMEA() const                 { return type == NMEA; }
        bool IsUBX() const                  { return type == UBX; }
        template<typename DT> const DT &As() const    { return static_cast<const DT &>(*this); }
        virtual bool IsValid() const        { return false; }
        virtual bool ChecksumValid() const  { return false; }
        virtual bool IsNew() const          { bool temp = isNew; isNew = false; return temp; }
        virtual uint32_t Age() const        { return Utils::msticks() - lastUpdateTime; }
        virtual uint32_t Timestamp() const  { return lastUpdateTime; }
        virtual std::string ToString() const = 0;
        virtual std::vector<uint8_t> ToBuffer() const = 0;
        virtual size_t Size() const = 0;
    };

    class UnknownPacket : public Packet
    {
        std::vector<uint8_t> buffer;
    public:
        virtual std::vector<uint8_t> ToBuffer() const    { return buffer; }
        virtual size_t Size() const                 { return buffer.size(); }
        virtual std::string ToString() const
        {
            char buf[64];
            sprintf(buf, "Unknown packet: size %d", buffer.size());
            return buf;
        }
        static UnknownPacket FromBuffer(std::vector<uint8_t> buffer) 
        {
            UnknownPacket packet;
            packet.buffer = buffer; 
            return packet;
        }
        static UnknownPacket Empty;
    };

    class NmeaPacket : public Packet
    {
        std::vector<std::string> fields;
        bool hasChecksum = false;
        bool checksumValid = false;
        size_t charCount = 0;
        
    public:
        /// @brief Indicates whether the sentence has reasonable NMEA construction
        /// @details Note: This method returns true if the sentence *has* a checksum, even if it's wrong
        /// @return true if sentence is valid NMEA
        virtual bool IsValid() const            { return fields.size() > 0 && fields[0].length() <= 6 && fields[0].length() > 1 && fields[0][0] == '$' && hasChecksum; }
        /// @brief Tests whether the NMEA checksum is correct
        /// @return true if Checksum is present and valid
        virtual bool ChecksumValid() const      { return checksumValid; }
        virtual std::string ToString() const;
        virtual std::vector<uint8_t> ToBuffer() const;
        virtual size_t Size() const             { return charCount; }

        std::string TalkerId() const                 { return IsValid() ? fields[0].substr(1, 2) : ""; }
        std::string SentenceId() const               { return IsValid() ? fields[0].substr(3) : ""; }
        void Clear()                            { fields.clear(); }
        std::string operator[](int field) const      { return field >= fields.size() ? "" : fields[field]; }
        size_t FieldCount() const               { return fields.size(); }

        static NmeaPacket FromString(const std::string &str);
        static NmeaPacket Empty;
    };

    struct Ubx
    {
        uint8_t sync[2];
        uint8_t clss;
        uint8_t id;
        std::vector<uint8_t> payload;
        uint8_t chksum[2];
    };

    class UbxPacket : public Packet
    {
        Ubx ubx;
        bool valid = false;
        bool checksumValid = false;
        mutable bool isNew = false;
        uint32_t lastUpdateTime = 0;
        size_t charCount = 0;

        static void updateChecksum(uint8_t &ck_A, uint8_t &ck_B, uint8_t b)
        {
            ck_A += b;
            ck_B += ck_A;
        }
        
    public:
        /// @brief Tests whether the NMEA checksum is correct
        /// @return true if Checksum is present and valid
        virtual bool IsValid() const            { return valid; }
        virtual bool ChecksumValid() const      { return checksumValid; }
        uint8_t Class() const                   { return ubx.clss; }
        uint8_t Id() const                      { return ubx.id; }
        void Clear()                            { ubx.clss = ubx.id = 0; ubx.payload.clear(); }
        virtual size_t Size() const             { return charCount; }
        const Ubx &Packet() const               { return ubx; }

        static UbxPacket FromUbx(const Ubx &ubx);
        virtual std::string ToString() const;
        virtual std::vector<uint8_t> ToBuffer() const;
        static UbxPacket Empty;
    };

    struct SentenceMap
    {
        std::map<std::string, NmeaPacket> AllSentences;
        const NmeaPacket &operator[] (const char *id) const 
        { 
            auto ret = AllSentences.find(id);
            return ret == AllSentences.end() ? NmeaPacket::Empty : ret->second;
        }
    };
}