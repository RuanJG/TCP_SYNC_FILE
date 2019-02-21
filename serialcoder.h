#ifndef SERIALCODER_H
#define SERIALCODER_H


#include "QByteArray"
#include "list"
#include "QDebug"


#define PACKET_START         0xAC
#define PACKET_END           0xAD
#define PACKET_ESCAPE        0xAE
#define PACKET_ESCAPE_MASK   0x80
#define MAX_PACKET_SIZE      (1024)

class SerialCoder
{
private:
    QByteArray mPackget;
    unsigned char crc8(const char *data, uint32_t size);
    void log(char* str);
    bool mStartTag;
    bool mEscTag;

public:
    SerialCoder();
    ~SerialCoder();
    QByteArray encode(const char * data, int size, bool withCRC=true);
    void clear();
    QByteArray parse(const char data, bool withCRC=true, bool *error=nullptr);
};

#endif // SERIALCODER_H
