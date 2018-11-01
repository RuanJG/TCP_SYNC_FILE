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
    mSetting("serverSetting.ini",QSettings::IniFormat),
    mDataStorer()
{
    ui->setupUi(this);
    on_serverOnOFFButton_clicked();

    QString PCfile = qApp->applicationDirPath()+"\/SBS_PCdata.txt";
    QString PADfile = qApp->applicationDirPath()+"\/SBS_PADdata.txt";

    mPCMsgBackupfile = qApp->applicationDirPath()+"\/SBS_PC_Msg_Backup.txt";
    mPADMsgBackupfile = qApp->applicationDirPath()+"\/SBS_PAD_Msg_Backup.txt";

    QString res;
    if( !mDataStorer.ReadInitPcDataFile(PCfile, res) )
    {
        QMessageBox::warning(this,tr("错误"), res);
        //qApp->quit();
    }



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


bool MainWindow::saveMsgToFile(QString file,QString msg)
{
    QFile savefile(file);
    if( !savefile.open(QFile::Append|QFile::Text)){
        //QMessageBox::warning(this,tr("错误"), tr("打开数据文件失败"));
        return false;
    }
    //savefile.seek(savefile.size());
    savefile.write(msg.toLocal8Bit());
    savefile.flush();
    savefile.close();
    return true;
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

bool MainWindow::pc_tester_data_check_and_store( TcpServerWorker *worker, QString id, QByteArray data)
{
    bool res = true;
    mPCDataMute.lock();

    QString str;
    str = QString::fromLocal8Bit(data);
    log("receive pc data:"+str);
    saveMsgToFile(mPCMsgBackupfile, str);

    //如果同一信息发两次，当是要覆盖
    bool replaced = worker->mLastMsg == str;
    log("new>"+str+"\n"+worker->mLastMsg);

    DataStorer::DATASTORER_ERROR_TYPE storeRes = mDataStorer.storePcDataFromPcMsg( str,replaced );
    switch(storeRes)
    {
    case DataStorer::ERROR_DATA:
        worker->sendAck(TcpServerWorker::ACKType::ERROR_DATA);
        break;
    case DataStorer::ERROR_FILE_ERROR:
        worker->sendAck(TcpServerWorker::ACKType::ERROR_FILE);
        break;
    case DataStorer::ERROR_REPEAT_BARCODE:
        worker->sendAck(TcpServerWorker::ACKType::ERROR_REPEAT_BARCODE);
        worker->mLastMsg = str;
        break;
    case DataStorer::ERROR_NONE:
        worker->sendAck(TcpServerWorker::ACKType::OK);
        break;
    }

    mPCDataMute.unlock();

    return res;
}

bool MainWindow::pad_tester_data_check_and_store(TcpServerWorker *worker, QString id, QByteArray data)
{
    bool res = true;
    mPADDataMute.lock();

    QString msg;
    msg = QString::fromLocal8Bit(data);

    worker->sendAck(TcpServerWorker::ACKType::OK);
    saveMsgToFile(mPADMsgBackupfile, msg);


    mPADDataMute.unlock();
    return res;
}

MainWindow::slot_worker_data_received( TcpServerWorker *worker,QByteArray buffer)
{
    //check the ID
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
        pc_tester_data_check_and_store(worker, worker->mClientID,buffer) ;
    }else if( worker->mClientType == TcpServerWorker::ClientType::PADTESTER ){
        pad_tester_data_check_and_store(worker,worker->mClientID,buffer);
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
