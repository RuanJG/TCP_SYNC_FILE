#include "datastorer.h"
#include <QCryptographicHash>

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
    if( data.count() != 5 )
        return false;

    //check barcode
    value = data.at(0);
    if( value.length() != 16) return false;
    //check
    value = data.at(1);
    if( value.length() != 2) return false;
    //check
    value = data.at(1);
    if( value.length() != 2) return false;
    //check
    value = data.at(1);
    if( value.length() != 2) return false;
    //check
    value = data.at(1);
    if( value.length() != 2) return false;

    pdata.barcode = data.at(0);
    pdata.mac =     data.at(1);
    pdata.temp_comp = data.at(2);
    pdata.Cali_ADC = data.at(3);
    pdata.UID =     data.at(4);


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
    int pos;

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

    int pos = mPcDataStoreFile.pos();
    if( isContainItem(pcdata.barcode))
    {
        if( !replaced )
            return ERROR_REPEAT_BARCODE;

        if( ! mPcDataStoreFile.seek(mPcBarcodeMap.value(pcdata.barcode)) ){
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
    mPcBarcodeMap.insert(pcdata.barcode,pos);
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
