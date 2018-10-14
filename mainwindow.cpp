#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mServer(NULL),
    mClientList(),
    mPCDataMute(),
    mPADDataMute(),
    mSetting("serverSetting.ini",QSettings::IniFormat)
{
    ui->setupUi(this);
    on_serverOnOFFButton_clicked();

}

MainWindow::~MainWindow()
{
    stop_server_listen();
    delete ui;
}

MainWindow::log(QString str)
{
    ui->Console->append(str);
}
MainWindow::clearLog()
{
    ui->Console->clear();
}



bool MainWindow::start_server_listen()
{
    int port = ui->portLineEdit->text().toInt();

    if( mServer != NULL ){
        stop_server_listen();
    }
    mServer = new QTcpServer();
    connect(mServer, &QTcpServer::newConnection, this, new_Client_Connect);
    if( !mServer->listen(QHostAddress::Any, port) ){
        QMessageBox::warning(this,tr("错误"),tr("TCP服务打开失败"),NULL,NULL);
        stop_server_listen();
        return false;
    }
    return true;
}

MainWindow::stop_server_listen()
{
    foreach (TcpServerWorker *worker, mClientList) {
        worker->closeSocket();
        //worker->mSocket->abort();
        //client->mSocket->deleteLater();
        delete worker;
    }
    mClientList.clear();

    if( mServer != NULL ){
        mServer->close();
        mServer->deleteLater();
        disconnect(mServer, &QTcpServer::newConnection, this, new_Client_Connect);
        mServer = NULL;
    }

    ui->ClientCountLabel->setText(QString::number(mClientList.count()));
}

MainWindow::new_Client_Connect()
{
    QTcpSocket *sock;
    TcpServerWorker *worker;

    worker = new TcpServerWorker();
    connect(worker,&TcpServerWorker::singal_data_received,this,slot_worker_data_received);
    connect(worker,&TcpServerWorker::singal_socket_abort,this,slot_worker_socket_abort);
    sock = mServer->nextPendingConnection();
    worker->setSocket( sock );
    mClientList.append(worker);

    ui->ClientCountLabel->setText(QString::number(mClientList.count()));
    log("new connection , totall "+QString::number(mClientList.count()));
}


MainWindow::server_setup_worker_id_type( TcpServerWorker *worker,QByteArray buffer )
{
    QString data;
    int type;
    data = QString::fromLocal8Bit(buffer);

    if( data.length() == 5 && "TYPE" == data.left(4) )
    {
        //setup type
        type = data.right(1).toInt();

        if( type > TcpServerWorker::ClientType::UNKNOW && type <= TcpServerWorker::ClientType::PADTESTER )
        {
           worker->mClientType = type;
           worker->sendAck(TcpServerWorker::ACKType::OK);
           log("receive type="+ QString::number(type) );
        }else{
           worker->sendAck(TcpServerWorker::ACKType::ERROR_TYPE);
        }
    }else if( buffer.length() == 3 && "ID" == data.left(2)){
        //setup id
        QString id = data.right(1);
        bool hassameid = false;
        foreach( TcpServerWorker* w , mClientList){
            if( w->mClientID == id) hassameid = true;
        }
        if( hassameid ){
            worker->sendAck(TcpServerWorker::ACKType::ERROR_ID);
        }else{
            worker->mClientID = id;
            worker->sendAck(TcpServerWorker::ACKType::OK);
            log("receive id="+ id );
        }
    }else{
        worker->sendAck(TcpServerWorker::ACKType::ERROR_SETUP_ID_TYPE);
    }
}

bool MainWindow::pc_tester_data_check_and_store( QString id, QByteArray data)
{
    bool res = true;
    mPCDataMute.lock();

    QString str;
    str = QString::fromLocal8Bit(data);
    log("receive pc data:"+str);


    mPCDataMute.unlock();

    return res;
}

bool MainWindow::pad_tester_data_check_and_store( QString id, QByteArray data)
{
    bool res = true;
    mPADDataMute.lock();




    mPADDataMute.unlock();
    return res;
}

MainWindow::slot_worker_data_received( TcpServerWorker *worker,QByteArray buffer)
{
    if( worker->mClientType == TcpServerWorker::ClientType::UNKNOW || worker->mClientID == ""){
        server_setup_worker_id_type( worker, buffer);
        if( worker->mClientType != TcpServerWorker::ClientType::UNKNOW && worker->mClientID != ""){
            QString name = worker->mClientType == TcpServerWorker::ClientType::PCTESTER ? "PC":"PAD";
            log("Client joined: "+name+worker->mClientID);
        }
        return 0;
    }

    // handle the data packget
    if( worker->mClientType == TcpServerWorker::ClientType::PCTESTER){
        if( pc_tester_data_check_and_store(worker->mClientID,buffer) ){
            worker->sendAck(TcpServerWorker::ACKType::OK);
        }else{
            worker->sendAck(TcpServerWorker::ACKType::ERROR_DATA);
        }
    }else if( worker->mClientType == TcpServerWorker::ClientType::PADTESTER ){
        if( pad_tester_data_check_and_store(worker->mClientID,buffer) ){
            worker->sendAck(TcpServerWorker::ACKType::OK);
        }else{
            worker->sendAck(TcpServerWorker::ACKType::ERROR_DATA);
        }
    }else{
        log(tr("客户端类型有错"));
        worker->sendAck(TcpServerWorker::ACKType::ERROR_DATA);
        QMessageBox::warning(this,tr("ERROR"),tr("客户端类型有错"),NULL,NULL);
    }


}

MainWindow::slot_worker_socket_abort( TcpServerWorker *worker)
{
    worker->closeSocket();
    //worker->mSocket->abort();
    //worker->mSocket->deleteLater();
    mClientList.removeOne(worker);
    delete worker;
    ui->ClientCountLabel->setText(QString::number(mClientList.count()));
}




void MainWindow::on_serverOnOFFButton_clicked()
{
    if( mServer != NULL){
        stop_server_listen();
        log("TCP server off");
        ui->serverOnOFFButton->setText(tr("开启服务"));
    }else{
        if( start_server_listen() ){
            log("TCP server start OK");
            ui->serverOnOFFButton->setText(tr("关闭服务"));
        }else{
            log("TCP server start False");
            ui->serverOnOFFButton->setText(tr("开启服务"));
        }
    }
}
