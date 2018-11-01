#ifndef TCPSERVERWORKER_H
#define TCPSERVERWORKER_H

#include <QObject>
#include <QTcpSocket>

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
    enum ClientType {
        UNKNOW = 1,
        PCTESTER = 2,
        PADTESTER = 3
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
signals:
    void singal_data_received(TcpServerWorker *worker, QByteArray data);
    void singal_socket_abort(TcpServerWorker *worker);
public slots:

    void slot_socket_data_received();
    void slot_socket_disconnect();
private:

};

#endif // TCPSERVERWORKER_H
