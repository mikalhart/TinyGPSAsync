namespace TinyGPS
{
    struct Statistics
    {
        uint32_t encodedCharCount = 0;
        uint32_t validSentenceCount = 0;
        uint32_t validUbxCount = 0;
        uint32_t invalidSentenceCount = 0;
        uint32_t invalidUbxCount = 0;
        uint32_t failedChecksumCount = 0;
        uint32_t passedChecksumCount = 0;
        uint32_t ggaCount = 0;
        uint32_t rmcCount = 0;
        uint32_t ubxNavPvtCount = 0;
        uint32_t ubx153Count = 0;

        void clear()
        {
            encodedCharCount = validSentenceCount = invalidSentenceCount = failedChecksumCount =
                passedChecksumCount = ggaCount = rmcCount = ubxNavPvtCount = ubx153Count = 0;
        }
    };
}
