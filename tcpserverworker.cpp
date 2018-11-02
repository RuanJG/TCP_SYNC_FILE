#include "tcpserverworker.h"

TcpServerWorker::TcpServerWorker(QObject *parent) :
    QObject(parent)
{
    mClientType = TcpServerWorker::ClientType::UNKNOW;
    mClientID = "";
    mSocket = NULL;
    mLastMsg = "";
}

TcpServerWorker::~TcpServerWorker()
{
    closeSocket();
}

TcpServerWorker::log(QString str)
{

}

QString TcpServerWorker::getClienName()
{
    QString name;

    if( mClientType == TcpServerWorker::ClientType::PCTESTER ) name="PC";
    if( mClientType == TcpServerWorker::ClientType::PADTESTER ) name="PAD";
    name += mClientID;
    return name;
}

void TcpServerWorker::setSocket( QTcpSocket *socket)
{
    closeSocket();
    mSocket = socket;
    this->connect(mSocket,&QTcpSocket::readyRead, this, slot_socket_data_received);
    this->connect(mSocket, &QTcpSocket::disconnected, this, slot_socket_disconnect);
}

void TcpServerWorker::closeSocket()
{
    if( mSocket == NULL) return;

    this->disconnect(mSocket,&QTcpSocket::readyRead, this, slot_socket_data_received);
    this->disconnect(mSocket, &QTcpSocket::disconnected, this, slot_socket_disconnect);
    //mClient->abort();
    //mClient->deleteLater();
    mSocket = NULL;
    mClientType = TcpServerWorker::ClientType::UNKNOW;
    mClientID = "";
    mLastMsg = "";

}

TcpServerWorker::sendAck(enum ACKType res)
{
    if( mSocket != NULL)
        mSocket->write(QString("ACK"+QString::number(res)).toLocal8Bit());
}

void TcpServerWorker::slot_socket_data_received()
{
    QByteArray buffer;

    buffer = mSocket->readAll();
    if( buffer.isEmpty() ) return ;

    emit singal_data_received(this, buffer);
}

void TcpServerWorker::slot_socket_disconnect()
{
    emit singal_socket_abort(this);
}


























