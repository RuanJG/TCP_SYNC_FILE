#ifndef TCPSERVERWORKER_H
#define TCPSERVERWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <serialcoder.h>

class TcpServerWorker : public QObject
{
    Q_OBJECT
public:
    explicit TcpServerWorker(QObject *parent = 0);
    ~TcpServerWorker();
    void setSocket(QTcpSocket *socket);
    void closeSocket();
    QString mClientID;
    int mClientType;
    QTcpSocket *mSocket;
    QString mLastMsg;
    SerialCoder mCoder;
    enum ClientType {
        UNKNOW = 1,
        PCTESTER = 2,
        PADTESTER = 3,
        PCTESTER_TC = 4,   // 做过温保后的测试
        PCTESTER_ADJUST = 5,
        TYPE_MAX
    };
    enum ACKType {
        OK = 0,
        ERROR_ID ,
        ERROR_TYPE ,
        ERROR_SETUP_ID_TYPE,
        ERROR_DATA,
        ERROR_REPEAT_BARCODE,
        ERROR_FILE
    };
    sendAck(enum ACKType res);
    log(QString str);
    QString getClienName();
    sendBytes(QByteArray data);
    sendRawBytes(QByteArray data);
signals:
    void singal_data_received(TcpServerWorker *worker, QByteArray data);
    void singal_socket_abort(TcpServerWorker *worker);
public slots:

    void slot_socket_data_received();
    void slot_socket_disconnect();
private:

};

#endif // TCPSERVERWORKER_H
