#ifndef DATASTORER_H
#define DATASTORER_H

#include <QObject>
#include <QFile>
#include <QStringList>
#include <QMap>
#include <QMessageBox>

class DataStorer : public QObject
{
    Q_OBJECT
public:
    explicit DataStorer(QObject *parent = 0);
    typedef enum _ERROR_Type{
        ERROR_NONE,
                ERROR_REPEAT_BARCODE ,
                ERROR_DATA,
                ERROR_FILE_ERROR
    }DATASTORER_ERROR_TYPE;
    typedef struct _PcData{
        QString date;
        QString barcode;
        QString leftInteval;
        QString rightInterval;
    }PcData;
    typedef struct _PadData{
        QString barcode;
        QString mac;
        QString temp_comp;
        QString Cali_ADC;
        QString UID;
    }PadData;

    DataStorer::DATASTORER_ERROR_TYPE storePcDataFromPcMsg(QString msg, bool replaced = false);
    bool isContainItem(QString id);
    bool ReadInitPcDataFile(QString dfile,QString &errorStr);
    bool MsgToPcData(QString line, PcData &pdata);
    QString PcDataToMsg(PcData pdata);
    QString getPCDataFile();
    QString PadDataToMsg(PadData pdata);
    bool MsgToPadData(QString msg, PadData &pdata);
    QString getPadDataFile();
    bool getFileMd5(QString file, QString &md5code);
signals:

public slots:

private:
    QMap <QString,int> mPcBarcodeMap;
    QMap <QString,int> mPadBarcodeMap;
    QFile     mPcDataStoreFile;
    QFile     mPadDataStoreFile;


};

#endif // DATASTORER_H
