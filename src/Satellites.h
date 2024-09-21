#include <vector>
#include <map>
using namespace std;

namespace TinyGPS
{
    struct SatelliteInfo
    {
        string talker_id;
        bool used;
        uint16_t prn;
        int8_t elevation;
        uint16_t azimuth;
        uint8_t snr;
    };
};