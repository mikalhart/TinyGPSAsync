#include <vector>
using namespace std;

namespace TinyGPS
{
    struct SatInfo
    {
        uint16_t prn;
        uint8_t elevation;
        uint16_t azimuth;
        uint8_t snr;
    };

    struct Satellites
    {
        vector<SatInfo> Sats;
        string Talker;
    };
};