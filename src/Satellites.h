#include <vector>
#include <map>
#include <string>
// using namespace std;

namespace TinyGPS
{
    struct SatelliteInfo
    {
        std::string talker_id;
        bool used;
        uint16_t prn;
        int8_t elevation;
        uint16_t azimuth;
        int8_t snr;
    };
};