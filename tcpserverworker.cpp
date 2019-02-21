#include "tcpserverworker.h"

TcpServerWorker::TcpServerWorker(QObject *parent) :
    QObject(parent),
    mCoder()
{
    mClientType = TcpServerWorker::ClientType::UNKNOW;
    mClientID = "";
    mSocket = NULL;
    mLastMsg = "";
    mCoder.clear();
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
    if( mClientType == TcpServerWorker::ClientType::PCTESTER_TC ) name="PC_TC";
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

TcpServerWorker::sendRawBytes(QByteArray data)
{
    if( mSocket != NULL)
        mSocket->write(data);
}

TcpServerWorker::sendBytes(QByteArray data)
{
    if( mSocket != NULL){
        mSocket->write(mCoder.encode(data.data(),data.length(),true));
    }
}

TcpServerWorker::sendAck(enum ACKType res)
{
    sendBytes(QString("ACK"+QString::number(res)).toLocal8Bit());
}

void TcpServerWorker::slot_socket_data_received()
{
    QByteArray buffer;
    QByteArray pkg;
    bool error;

    buffer = mSocket->readAll();
    for( int i=0; i< buffer.length(); i++)
    {
        pkg = mCoder.parse(buffer.at(i),true,&error);
        if( error ){
            qDebug()<<("slot_worker_data_received: Coder parse error!!");
        }else{
            if( pkg.length() > 0 ){
                emit singal_data_received(this, pkg);
            }
        }
    }
}

void TcpServerWorker::slot_socket_disconnect()
{
    emit singal_socket_abort(this);
}


























