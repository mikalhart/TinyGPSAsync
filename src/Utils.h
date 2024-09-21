#include <chrono>
#pragma once
using namespace std::chrono;

namespace TinyGPS
{
    class Utils
    {
    public:
        // Function to replace Arduino's millis()
        static unsigned long msticks() 
        {
            static auto start_time = high_resolution_clock::now();
            auto current_time = high_resolution_clock::now();

            // Calculate the difference in milliseconds
            return duration_cast<milliseconds>(current_time - start_time).count();
        }
    };
}