package com.rompa.jk.tcpfilesyncer;

import android.util.Log;

/**
 * Created by RSL-030 on 2019/2/21.
 */

public class SerialCoder {
    public static final byte PACKET_START     = (byte) 0xAC;
    public static final byte PACKET_END         = (byte)  0xAD;
    public static final byte PACKET_ESCAPE       = (byte) 0xAE;
    public static final byte PACKET_ESCAPE_MASK  = (byte) 0x80;
    public static final int  MAX_PACKET_SIZE     = (1024);

    private  volatile byte[]mPackget = new byte[MAX_PACKET_SIZE+1];
    private  volatile int mLength = 0;
    private volatile boolean mStartTag;
    private volatile boolean mEscTag;

    SerialCoder()
    {
        clear();
    }


    byte[] subBytes(byte[] data, int startIndex, int len)
    {
        byte[] sData = new byte[len];
        System.arraycopy(data, startIndex, sData, 0, len);
        return sData;
    }

    byte[] parse(byte pData ,boolean withCRC)
    {
        //log("0x"+Integer.toHexString(pData&0xff));
        byte[] pack = new byte[0];

        if( pData == PACKET_START )
        {
            //log("get start");
            mStartTag = true;
            mEscTag = false;
            if( mLength > 0){
                mLength=0;
                log("delete some unuseful data");
            }
            return pack;
        }


        if ( pData == PACKET_END )
        {
            //log("get end");
            if( ((withCRC && mLength>1) || (!withCRC && mLength>0))  && mStartTag && !mEscTag){
                if( withCRC ) {
                    byte crc = crc8(mPackget ,mLength-1);
                    if( mPackget[mLength-1] != crc){
                        log("SerialCoder : error crc");
                    }else{
                        pack = subBytes(mPackget,0,mLength-1);
                    }
                }else{
                    pack = subBytes(mPackget,0,mLength);
                }
            }else{
                log("SerialCoder : error  packget");
            }
            mStartTag = false;
            mEscTag = false;
            mLength=0;
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
                //log("get data");
                mPackget[mLength]=(pData);
                mLength++;
                if( mLength > MAX_PACKET_SIZE){
                    mStartTag = false;
                    mEscTag = false;
                    mLength=0;
                    log("SerialCoder : error packget too big");
                }
            }
        }

        return pack;

    }


    void  clear()
    {
        mLength=0;
        mStartTag = false;
        mEscTag = false;
    }

    byte[] encode(byte[] data, int size , boolean withCRC)
    {
        byte[] pkg = new byte[size*2+5];
        int index = 0;

        pkg[index++] = (PACKET_START);
        for( int i=0; i<size; i++)
        {
            if( data[i] == PACKET_END || data[i] == PACKET_START || data[i] == PACKET_ESCAPE){
                pkg[index++] = (PACKET_ESCAPE);
                pkg[index++] = (byte)(PACKET_ESCAPE_MASK ^ data[i]);
            }else{
                pkg[index++] = (data[i]);
            }
        }
        if( withCRC ){
            byte crc = crc8(data,size) ;
            if( crc == PACKET_END || crc == PACKET_START || crc == PACKET_ESCAPE){
                pkg[index++] = (PACKET_ESCAPE);
                pkg[index++] = (byte)(PACKET_ESCAPE_MASK ^ crc);
            }else{
                pkg[index++] = (crc);
            }
        }

        pkg[index++]= (PACKET_END);
        return pkg;
    }


    byte crc8(byte[] data, int size) {
        int i = 0;
        int j = 0;
        int crc = 0;

        for (i = 0; i < size; ++i) {
            crc = 0xff & ( crc ^ data[i]);
            for (j = 0; j < 8; ++j) {
                if ((crc & 0x01) != 0) {
                    crc = 0xff & ((crc >> 1) ^ 0x8C);
                } else {
                    crc >>= 1;
                }
            }
        }
        return (byte)crc;
    }

    void log(String str)
    {
        Log.e("Ruan",str);
    }



}
