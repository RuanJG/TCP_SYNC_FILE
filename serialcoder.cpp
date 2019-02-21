#include "serialcoder.h"

SerialCoder::SerialCoder()
{
    clear();
}

SerialCoder::~SerialCoder()
{
    clear();
}


QByteArray SerialCoder::parse(const char data ,bool withCRC,bool *error)
{
    unsigned char pData = (unsigned char) data;
    QByteArray pack;
    bool errorb;

    if( error == nullptr) error = &errorb;
    *error =false;

    if( pData == PACKET_START )
    {
        mStartTag = true;
        mEscTag = false;
        if( mPackget.size() > 0){
            mPackget.clear();
            *error =true;
        }
        return pack;
    }


    if ( pData == PACKET_END )
    {
        if( ((withCRC && mPackget.size()>1) || (!withCRC && mPackget.size()>0))  && mStartTag && !mEscTag){
            if( withCRC ) {
                unsigned char crc = crc8(mPackget.data(),mPackget.size()-1);
                if( (unsigned char)mPackget.at(mPackget.size()-1) != crc){
                    log("error crc: ");
                    *error =true;
                }else{
                    pack = mPackget.left(mPackget.size()-1);
                }
            }else{
                pack = mPackget;
            }
        }else{
            log("error  packget");
            *error =true;
        }
        mStartTag = false;
        mEscTag = false;
        mPackget.clear();
    }

    if( mStartTag)//data
    {
        if( pData == PACKET_ESCAPE){
            mEscTag = true;
        }else{
            if( mEscTag ){
                pData ^= PACKET_ESCAPE_MASK;
                mEscTag = false;
            }
            mPackget.append(pData);
            if( mPackget.size() > MAX_PACKET_SIZE){
                mStartTag = false;
                mEscTag = false;
                mPackget.clear();
            }
        }
    }

    return pack;

}


void SerialCoder::clear()
{
    mPackget.clear();
    mStartTag = false;
    mEscTag = false;
}

QByteArray SerialCoder::encode(const char *pdata, int size , bool withCRC)
{
    QByteArray pkg;
    unsigned char *data = (unsigned char*)pdata;

    pkg.append(PACKET_START);
    for( int i=0; i<size; i++)
    {
        if( data[i] == PACKET_END || data[i] == PACKET_START || data[i] == PACKET_ESCAPE){
            pkg.append(PACKET_ESCAPE);
            pkg.append(PACKET_ESCAPE_MASK ^ data[i]);
        }else{
            pkg.append(data[i]);
        }
    }
    if( withCRC ){
        unsigned char crc = crc8(pdata,size) ;
        if( crc == PACKET_END || crc == PACKET_START || crc == PACKET_ESCAPE){
            pkg.append(PACKET_ESCAPE);
            pkg.append(PACKET_ESCAPE_MASK ^ crc);
        }else{
            pkg.append(crc);
        }
    }

    pkg.append(PACKET_END);
    return pkg;
}


unsigned char SerialCoder::crc8(const char *buff, uint32_t size) {
    uint32_t i = 0;
    uint32_t j = 0;
    uint8_t crc = 0;
    uint8_t *data = (uint8_t *)buff;

    for (i = 0; i < size; ++i) {
        crc = crc ^ data[i];
        for (j = 0; j < 8; ++j) {
            if ((crc & 0x01) != 0) {
                crc = (crc >> 1) ^ 0x8C;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void SerialCoder::log(char* str)
{
    qDebug()<<str;
}
