#include <vector>
#include <map>
using namespace std;

class ParsedSentence
{
private:
    vector<string> fields;
    bool hasChecksum = false;
    bool checksumValid = false;
    mutable bool isNew = false;
    uint32_t lastUpdateTime = 0;
    uint8_t charCount = 0;
    
public:
    bool IsValid() const            { return fields.size() > 0  && fields[0].length() == 6 && fields[0][0] == '$' && hasChecksum; }
    bool ChecksumValid() const      { return checksumValid; }
    bool IsNew() const              { bool temp = isNew; isNew = false; return temp; }
    uint32_t Age() const            { return millis() - lastUpdateTime; }
    uint32_t Timestamp() const      { return lastUpdateTime; }
    string TalkerId() const         { return IsValid() ? fields[0].substr(1, 2) : ""; }
    string SentenceId() const       { return IsValid() ? fields[0].substr(3) : ""; }
    void Clear()                    { fields.clear(); }
    string operator[](int field) const   { return field >= fields.size() ? "" : fields[field]; }
    uint8_t CharCount() const       { return charCount; }

    static ParsedSentence FromString(const string &str);
    string String() const;
};

struct Sentences
{
    ParsedSentence LastSentence;
    std::map<string, ParsedSentence> AllSentences;
    const ParsedSentence &operator[] (const char *id) const { return AllSentences.at(id); }
};

