#include "datastorer.h"
#include <QCryptographicHash>
#include <QDebug>

DataStorer::DataStorer(QObject *parent) :
    QObject(parent),
    mPcBarcodeMap(),
    mPadBarcodeMap(),
    mPcDataStoreFile(),
    mPadDataStoreFile()
{

}


bool DataStorer::MsgToPcData(QString line, PcData &pdata)
{
    QStringList data;
    QString value;

    //line.remove("\n");
    data = line.split(QRegExp("\\s+"),QString::SkipEmptyParts);

    if( data.count() != 4 )
        return false;

    //check date
    value = data.at(0);
    if( value.length() != 14) return false;
    //check barcode
    value = data.at(1);
    if( value.length() != 16) return false;
    value = data.at(2);
    if( value.length() != 4) return false;
    value = data.at(3);
    if( value.length() != 4) return false;

    pdata.date = data.at(0);
    pdata.barcode = data.at(1);
    pdata.leftInteval = data.at(2);
    pdata.rightInterval = data.at(3);

    return true;
}

QString DataStorer::PcDataToMsg(PcData pdata)
{
    QString msg;
    msg = pdata.date +" " +pdata.barcode +" "+pdata.leftInteval+" "+pdata.rightInterval +"\n";
    return msg;
}

bool DataStorer::MsgToPadData(QString msg, PadData &pdata)
{
    QStringList data;
    QString value;

    //msg.remove("\n");
    data = msg.split(QRegExp("\\s+"),QString::SkipEmptyParts);
    qDebug()<<data;
    if( data.count() != 6 )
        return false;

    //check barcode
    value = data.at(0);
    if( value.length() != 16) return false;
    //check
    value = data.at(1);
    if( value.length() != 17) return false;

/*
    //check
    value = data.at(2);
    if( value.length() != 2) return false;
    //check
    value = data.at(3);
    if( value.length() != 2) return false;
    //check
    value = data.at(4);
    if( value.length() != 2) return false;
    //check
    value = data.at(5);
    if( value.length() != 2) return false;
 */

    pdata.barcode = data.at(0);
    pdata.mac =     data.at(1);
    pdata.temp_comp = data.at(2);
    pdata.Cali_ADC = data.at(3);
    pdata.Loadcellzero = data.at(4);
    pdata.UID =     data.at(5);


    return true;
}

QString DataStorer::PadDataToMsg(PadData pdata)
{
    QString msg;
    msg = pdata.barcode +" "+pdata.mac+" "+pdata.temp_comp+" "+pdata.Cali_ADC+" "+pdata.UID +"\n";
    return msg;
}

QString DataStorer::getPCDataFile()
{
    return mPcDataStoreFile.fileName();
}

QString DataStorer::getPadDataFile()
{
    return mPadDataStoreFile.fileName();
}

bool DataStorer::ReadInitPcDataFile(QString dfile, QString &errorStr)
{
    QString line;
    QStringList list;
    PcData pcdata;
    qint64 pos;

    if( mPcDataStoreFile.isOpen() ) mPcDataStoreFile.close();
    mPcDataStoreFile.setFileName(dfile);
    if( !mPcDataStoreFile.open(QFile::ReadWrite|QFile::Text)){
        errorStr =  tr("打开PC数据文件失败");
        return false;
    }

    mPcBarcodeMap.clear();
    while(true){
       pos = mPcDataStoreFile.pos();
       line = QString(mPcDataStoreFile.readLine(100));
       if( line.length() <= 1) break;

       if( !MsgToPcData(line,pcdata) ){
           errorStr = tr("是否忽略－PC数据文件解释错误:行")+QString::number(mPcBarcodeMap.count()+1)+"\n"+line;
           mPcBarcodeMap.clear();
           return false;

       }

       if( mPcBarcodeMap.contains(pcdata.barcode) )
       {
            errorStr = tr("发现重复序列号!!\n行")+QString::number(mPcBarcodeMap.count()+1)+"\n"+line;
            mPcBarcodeMap.clear();
            return false;
       }
       mPcBarcodeMap.insert(pcdata.barcode,pos);
    }

    return true;
}

bool DataStorer::isContainItem(QString id)
{
    return mPcBarcodeMap.contains(id);
}

DataStorer::DATASTORER_ERROR_TYPE DataStorer::storePcDataFromPcMsg(QString msg, bool replaced)
{
    QString line;
    QStringList list;
    PcData pcdata;

    if( !mPcDataStoreFile.isOpen() ) return DataStorer::ERROR_FILE_ERROR;

    if( ! MsgToPcData(msg ,pcdata) ){
        return ERROR_DATA;
    }

    qint64 pos = mPcDataStoreFile.pos();
    qint64 insert_pos = pos;
    if( isContainItem(pcdata.barcode))
    {
        if( !replaced )
            return ERROR_REPEAT_BARCODE;

        insert_pos = mPcBarcodeMap.value(pcdata.barcode);
        if( ! mPcDataStoreFile.seek(insert_pos) ){
            mPcDataStoreFile.seek(pos);
            return DataStorer::ERROR_FILE_ERROR;
        }
        if( -1 == mPcDataStoreFile.write(msg.toLocal8Bit()) ){
            return DataStorer::ERROR_FILE_ERROR;
        }
        mPcDataStoreFile.seek(pos);
    }else{
        if( -1 == mPcDataStoreFile.write(msg.toLocal8Bit()) ){
            return DataStorer::ERROR_FILE_ERROR;
        }
    }

    mPcDataStoreFile.flush();
    mPcBarcodeMap.insert(pcdata.barcode,insert_pos);
    return ERROR_NONE;
}



bool DataStorer::getFileMd5(QString file, QString &md5code)
{
    QFile localFile(file);

    if (!localFile.open(QFile::ReadOnly))
    {
        return false;
    }

    QCryptographicHash ch(QCryptographicHash::Md5);
    quint64 loadSize = 1024;
    QByteArray buf;
    while (true)
    {
        buf = localFile.read(loadSize);
        if( buf.length() == 0) break;
        ch.addData(buf);
    }
    md5code = QString(ch.result().toHex());

    localFile.close();

    return true;
}


bool DataStorer::storePadDataFromPcMsg(QString file, qint64 pos, QByteArray data)
{
    QFile pfile(file);

    if( ! pfile.open(QFile::ReadWrite) ){
        qDebug()<<"!!!!!!! open file "+file+" false";
        return false;
    }
    if( !pfile.seek(pos)){
        qDebug()<<"!!!!!!! file seek to "+QString::number(pos)+" false";
        pfile.close();
        return false;
    }
    if( -1 == pfile.write(data) ){
        qDebug()<<"!!!!!!! file write data false";
        pfile.close();
        return false;
    }

    pfile.close();
    return true;
}

QByteArray  DataStorer::intToQByteArray(int i)
{
    QByteArray abyte0;
    abyte0.resize(4);
    abyte0[0] = (uchar)  (0x000000ff & i);
    abyte0[1] = (uchar) ((0x0000ff00 & i) >> 8);
    abyte0[2] = (uchar) ((0x00ff0000 & i) >> 16);
    abyte0[3] = (uchar) ((0xff000000 & i) >> 24);
    return abyte0;
}

int DataStorer::QByteArrayToInt(QByteArray bytes)
{
    int addr = bytes[0] & 0x000000FF;
    addr |= ((bytes[1] << 8) & 0x0000FF00);
    addr |= ((bytes[2] << 16) & 0x00FF0000);
    addr |= ((bytes[3] << 24) & 0xFF000000);
    return addr;
}

