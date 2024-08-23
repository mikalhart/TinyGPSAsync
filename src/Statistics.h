struct Statistics
{
    uint32_t encodedCharCount = 0;
    uint32_t validSentenceCount = 0;
    uint32_t invalidSentenceCount = 0;
    uint32_t failedChecksumCount = 0;
    uint32_t passedChecksumCount = 0;
    uint32_t ggaCount = 0;
    uint32_t rmcCount = 0;

    void Clear()
    {
        encodedCharCount = validSentenceCount = invalidSentenceCount = failedChecksumCount =
            passedChecksumCount = ggaCount = rmcCount = 0;
    }
};

