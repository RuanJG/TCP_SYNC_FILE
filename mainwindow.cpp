#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"
#include <QTimer>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <QHostInfo>
#include "serialcoder.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mServer(NULL),
    mClientList(),
    mPCDataMute(),
    mPADDataMute(),
    mSetting("serverSetting.ini",QSettings::IniFormat),
    mDataStorer(),
    mTCDataStorer(),
    mAdjustedDataStorer(),
    mExcel()
{
    ui->setupUi(this);
    on_serverOnOFFButton_clicked();

    QHostInfo info = QHostInfo::fromName(QHostInfo::localHostName());
    ui->Console->append("Server HostName:"+QHostInfo::localHostName());
    ui->Console->append("Server IP:");
    foreach(QHostAddress address,info.addresses())
    {
        if(address.protocol() == QAbstractSocket::IPv4Protocol){
            ui->Console->append(address.toString());
        }
    }

    createDir(qApp->applicationDirPath()+"\/Data");
    QString PCfile = qApp->applicationDirPath()+"\/Data\/SBS_GapData.txt";
    QString PC_TC_file = qApp->applicationDirPath()+"\/Data\/SBS_GapData_TemptureCompensation.txt";
    QString PC_Adjust_file = qApp->applicationDirPath()+"\/Data\/SBS_GapData_Adjusted.txt";


    mPCMsgBackupfile = qApp->applicationDirPath()+"\/Data\/SBS_PC_Msg_Backup.txt";
    mPADMsgBackupfile = qApp->applicationDirPath()+"\/Data\/SBS_PAD_Msg_Backup.txt";

    QString res;
    if( !mDataStorer.ReadInitPcDataFile(PCfile, res) )
    {
        QMessageBox::warning(this,tr("错误"), res);
        QTimer::singleShot(0,qApp,SLOT(quit()));
    }

    mPC_TC_MsgBackupfile = qApp->applicationDirPath()+"\/Data\/SBS_PC_TC_Msg_Backup.txt";
    if( !mTCDataStorer.ReadInitPcDataFile(PC_TC_file, res) )
    {
        QMessageBox::warning(this,tr("打开温补Gap文件错误"), res);
        QTimer::singleShot(0,qApp,SLOT(quit()));
    }

    mPC_Adjusted_MsgBackupfile = qApp->applicationDirPath()+"\/Data\/SBS_PC_Adjusted_Msg_Backup.txt";
    if( !mAdjustedDataStorer.ReadInitPcDataFile(PC_Adjust_file, res))
    {
        QMessageBox::warning(this,tr("打开调校Gap文件错误"), res);
        QTimer::singleShot(0,qApp,SLOT(quit()));
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
bool MainWindow::createDir(const QString &path)
{
    QDir dir(path);
    if(dir.exists())
    {
        return true;
    }else{
        dir.setPath("");
        bool ok = dir.mkpath(path);
        return ok;
    }
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

        if( type > TcpServerWorker::ClientType::UNKNOW && type < TcpServerWorker::ClientType::TYPE_MAX )
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

        if( hassameid && worker->mClientID != id){
            worker->sendAck(TcpServerWorker::ACKType::ERROR_ID);
        }else{
            worker->mClientID = id;
            worker->sendAck(TcpServerWorker::ACKType::OK);
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
    //log("receive pc data:"+str);
    if( worker->mClientType == TcpServerWorker::ClientType::PCTESTER){
        saveMsgToFile(mPCMsgBackupfile, worker->getClienName()+" "+str);
    }else if( worker->mClientType == TcpServerWorker::ClientType::PCTESTER_TC){
        saveMsgToFile(mPC_TC_MsgBackupfile, worker->getClienName()+" "+str);
    }else if( worker->mClientType == TcpServerWorker::ClientType::PCTESTER_ADJUST){
        saveMsgToFile(mPC_Adjusted_MsgBackupfile, worker->getClienName()+" "+str);
    }

    //如果同一信息发两次，当是要覆盖
    bool replaced = worker->mLastMsg == str;


    DataStorer::DATASTORER_ERROR_TYPE storeRes;
    if(worker->mClientType == TcpServerWorker::ClientType::PCTESTER){
        storeRes= mDataStorer.storePcDataFromPcMsg( str,replaced );
    }else if ( worker->mClientType == TcpServerWorker::ClientType::PCTESTER_ADJUST){
        storeRes = mAdjustedDataStorer.storePcDataFromPcMsg(str,replaced);
    }else if ( worker->mClientType == TcpServerWorker::ClientType::PCTESTER_TC){
        storeRes= mTCDataStorer.storePcDataFromPcMsg( str,replaced );
    }else{
        storeRes= DataStorer::ERROR_DATA;
    }

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


QString MainWindow::getPadDataFilePath(QString clientname)
{
    return  QString(qApp->applicationDirPath()+"\/Data\/SBS_PADdata"+clientname+".txt");
}

QString MainWindow::getPadDataBackupFilePath(QString clientname)
{
    QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    return  QString(qApp->applicationDirPath()+"\/Data\/SBS_PADdata"+clientname+"_"+date+".txt");
}




//1,change tag to str
//2,improve the packget head and end tag
bool MainWindow::pad_tester_data_check_and_store(TcpServerWorker *worker, QString id, QByteArray msgdata)
{
    qint64 tag;
    bool ok;

    //[tag][#][data]
    int index = msgdata.indexOf("#");

    tag = QString(msgdata.left(index)).toLongLong(&ok);

    if( ok ){
        msgdata.remove(0,index+1);
        pad_tester_data_store(worker,tag,msgdata);
    }else{
        log("message error ,tag is not a number !!!");
        return false;
    }

    return true;
}

bool MainWindow::pad_tester_data_store(TcpServerWorker *worker, qint64 tag, QByteArray msg)
{
    bool res = true;
    mPADDataMute.lock();

    QFileInfo finfo(getPadDataFilePath(worker->getClienName()));
    QByteArray pkg;
    QString myMD5;
    long padbackupfilesize;
    QString filelen;

    log("get tag="+QString::number(tag));
    switch( tag )
    {
    case MainWindow::MD5_CODE_TAG:
        //pad send it's file md5(string) here, and I compare with my md5 , return the result
        mDataStorer.getFileMd5( getPadDataFilePath(worker->getClienName()), myMD5);
        if( myMD5 == QString(msg)){
            worker->sendAck(TcpServerWorker::ACKType::OK);
        }else{
            worker->sendAck(TcpServerWorker::ACKType::ERROR_DATA);
        }
        break;
    case MainWindow::FILE_SIZE_TAG:
        //ask for data
        pkg = mDataStorer.intToQByteArray(MainWindow::FILE_SIZE_TAG);
        //pkg = QString(QString::number(MainWindow::FILE_SIZE_TAG)+"#").toLocal8Bit();
        padbackupfilesize = QString(msg).toLongLong();
        if( padbackupfilesize < finfo.size() ){
            //备份本机的数据
            if( QFile::rename(getPadDataFilePath(worker->getClienName()), getPadDataBackupFilePath(worker->getClienName())) ){
                pkg = pkg + mDataStorer.intToQByteArray(0);//QString::number(0).toLocal8Bit();
            }else{
                log("重命名pad数据文件失败");
                pkg = pkg + mDataStorer.intToQByteArray(finfo.size());//QString::number(finfo.size()).toLocal8Bit();
            }
        }else{
            pkg = pkg + mDataStorer.intToQByteArray(finfo.size());//QString::number(finfo.size()).toLocal8Bit();
        }
        worker->sendBytes(pkg);
        break;
    default:
        if( tag < 0 ){
            log(tr("未知的Tag<")+worker->getClienName());
            worker->sendAck(TcpServerWorker::ACKType::ERROR_DATA);
            return false;
        }
        //file pos
        if( tag > finfo.size() ){
            log(tr("文件数据指针过大<")+worker->getClienName());
            worker->sendAck(TcpServerWorker::ACKType::ERROR_DATA);
            return false;
        }

        if( ! mDataStorer.storePadDataFromPcMsg(getPadDataFilePath(worker->getClienName()), tag, msg))
        {
            log(tr("文件写数据错误"));
            worker->sendAck(TcpServerWorker::ACKType::ERROR_FILE);
        }else{
            log("filefram pos="+QString::number(tag)+",data="+QString(msg));
            worker->sendAck(TcpServerWorker::ACKType::OK);
        }

        break;
    }

    mPADDataMute.unlock();
    return res;
}


MainWindow::slot_worker_data_received( TcpServerWorker *worker,QByteArray buffer)
{
    //check the ID
    if( worker->mClientType == TcpServerWorker::ClientType::UNKNOW || worker->mClientID == ""){
        server_setup_worker_id_type( worker, buffer);
        if( worker->mClientType != TcpServerWorker::ClientType::UNKNOW && worker->mClientID != ""){
            log("Client joined: "+worker->getClienName());
        }
        return 0;
    }

    // handle the data packget
    if( worker->mClientType == TcpServerWorker::ClientType::PCTESTER){
        pc_tester_data_check_and_store(worker, worker->mClientID,buffer) ;
    }else if( worker->mClientType == TcpServerWorker::ClientType::PADTESTER ){
        pad_tester_data_check_and_store(worker,worker->mClientID,buffer);
    }else if( worker->mClientType == TcpServerWorker::ClientType::PCTESTER_TC){
        pc_tester_data_check_and_store(worker, worker->mClientID,buffer) ;
    }else if( worker->mClientType == TcpServerWorker::ClientType::PCTESTER_ADJUST){
        pc_tester_data_check_and_store(worker, worker->mClientID,buffer) ;
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

void MainWindow::on_mergepushButton_clicked()
{
    QString filename = qApp->applicationDirPath()+"/Data/SBS_Data_"+QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss")+".xlsx";
    filename =QDir::toNativeSeparators(filename );

    if( mExcel.IsOpen() ) mExcel.Close();


    srand((int)QDateTime::currentMSecsSinceEpoch());
    int len = rand()% MAX_PACKET_SIZE;
    len = len>1 ? len:1;

    QByteArray ddd;
    for( int i=0 ;i<len; i++){
        ddd.append(rand()%0xff);
    }
    ddd[len/2] = 0xAC;

    QByteArray f,b;
    SerialCoder coder;
    b = coder.encode(ddd.data(),ddd.size(),true);
    for( int i=0; i< b.size(); i++){
        f = coder.parse(b.at(i),true);
    }

    bool ok = true;
    if( f.size() == len){
    for( int i=0 ;i<len; i++){

        if( ddd.at(i) != f.at(i)){
            ok = false;
        }
    }
    }else{
        ok = false;
    }
    if( ok ){
        log(ok?"OK!!":"false");
    }else{
        qDebug()<<"DDD:\n"<<ddd<<"\n";
        qDebug()<<"BBB:\n"<<b<<"\n";
        qDebug()<<"FFF:\n"<<f<<"\n";
    }
    return;



    log(tr("初始化Excel..."));
    if(!mExcel.Open(filename,1,false)){
        QMessageBox::warning(this,"Warning",tr("打开Excel文档失败"));
        return;
    }

    //set title
    if( mExcel.SetCellData(1, 1, "Serialnumber") &&
            mExcel.SetCellData(1, 2, "MAC") &&
            mExcel.SetCellData(1, 3, "TempComp") &&
            mExcel.SetCellData(1, 4, "Calibration") &&
            mExcel.SetCellData(1, 5, "Loadcellzero") &&
            mExcel.SetCellData(1, 6, "UID") &&
            mExcel.SetCellData(1, 7, "LeftGap") &&
            mExcel.SetCellData(1, 8, "RightGap") &&
            mExcel.SetCellData(1, 9, "LeftGap(before TC)") &&
            mExcel.SetCellData(1, 10, "RightGap(before TC)")){

            log("Excel file setup OK, start fill data...");
    }else{
        QMessageBox::warning(this,"Error",tr("Excel文档添加Title失败"));
        return;
    }

    // 先填Gap_befor_TC
    log(tr("开始读温补前的数据..."));
    QFile gapFile ;
    gapFile.setFileName(mDataStorer.getPCDataFile());
    QMap <QString,qint64> gapExcelMap;
    QString line;
    qint64 currentLine = 0;
    DataStorer::PcData gapData;
    qint64 excelLinenumber = 1;
    if( !gapFile.open(QFile::ReadOnly|QFile::Text)){
        QMessageBox::warning(this,"Error",tr("打开温补前的Gap数据文件失败"));
        return;
    }
    gapExcelMap.clear();
    while(true){
       line = QString(gapFile.readLine(100));
       if( line.length() <= 1) break;

       currentLine++;
       if( !mDataStorer.MsgToPcData(line,gapData) ){
           if( QMessageBox::Yes == QMessageBox::warning(this,"Warn",tr("解释错误，是否忽略此温补前Gap数据：行")+QString::number(currentLine),QMessageBox::Yes,QMessageBox::No))
           {
                continue;
           }else{
               gapFile.close();
                return;
           }
       }
       if( gapExcelMap.contains(gapData.barcode) ){
           if( QMessageBox::No == QMessageBox::warning(this,"Warn",tr("有重复的温补前Gap数据，是否覆盖？"),QMessageBox::Yes,QMessageBox::No))
           {
                continue;
           }
       }

       excelLinenumber++;
       gapExcelMap.insert(gapData.barcode,excelLinenumber);
       if( ! mExcel.SetCellData(excelLinenumber, 1, gapData.barcode) ||
               !mExcel.SetCellData(excelLinenumber, 9, gapData.leftInteval) ||
               !mExcel.SetCellData(excelLinenumber, 10, gapData.rightInterval)){
           QMessageBox::warning(this,"Error",tr("Excel写入温补前Gap数据失败"));
           gapFile.close();
           return;
       }
    }
    gapFile.close();


    // gap after tc
    log(tr("开始读温补后的数据..."));
    QFile gapTCFile;
    gapTCFile.setFileName(mTCDataStorer.getPCDataFile());
    currentLine = 0;
    if( !gapTCFile.open(QFile::ReadOnly|QFile::Text)){
        QMessageBox::warning(this,"Error",tr("打开温补后的Gap数据文件失败"));
        return;
    }
    while(true){
       line = QString(gapTCFile.readLine(100));
       if( line.length() <= 1) break;

       currentLine++;
       if( !mDataStorer.MsgToPcData(line,gapData) ){
           if( QMessageBox::Yes == QMessageBox::warning(this,"Warn",tr("解释错误，是否忽略此温补前Gap数据：行")+QString::number(currentLine),QMessageBox::Yes,QMessageBox::No))
           {
                continue;
           }else{
               gapTCFile.close();
                return;
           }
       }
       if( !gapExcelMap.contains(gapData.barcode) ){
           excelLinenumber++;
           gapExcelMap.insert(gapData.barcode,excelLinenumber);
           if( ! mExcel.SetCellData(excelLinenumber, 1, gapData.barcode) ||
                   !mExcel.SetCellData(excelLinenumber, 7, gapData.leftInteval) ||
                   !mExcel.SetCellData(excelLinenumber, 8, gapData.rightInterval)){
               QMessageBox::warning(this,"Error",tr("Excel添加温补后Gap数据失败"));
               gapTCFile.close();
               return;
           }
       }else{
           qint64 row = gapExcelMap.value(gapData.barcode);
           if(     !mExcel.SetCellData(row, 7, gapData.leftInteval) ||
                   !mExcel.SetCellData(row, 8, gapData.rightInterval)){
               QMessageBox::warning(this,"Error",tr("Excel写入温补后Gap数据失败"));
               gapTCFile.close();
               return;
           }
       }
    }



    //fill pad data
    log(tr("开始读检校数据..."));
    QDir fromDir = qApp->applicationDirPath()+"\/Data";
    QStringList filters;
    filters.append("SBS_PADdata*.txt");
    QFileInfoList fileInfoList = fromDir.entryInfoList(filters, QDir::AllDirs|QDir::Files);
    QStringList padDataFileList;
    foreach(QFileInfo fileInfo, fileInfoList)
    {
        if (fileInfo.fileName() == "." || fileInfo.fileName() == "..")
           continue;
        if (!fileInfo.isDir())
        {
            padDataFileList.append(fileInfo.absoluteFilePath());
            //fileNameList.append(fileInfo.fileName());
        }
    }

    DataStorer::PadData paddata;
    foreach(QString padDataFilepath, padDataFileList)
    {
        QFile padDataFile ;
        padDataFile.setFileName(padDataFilepath);
        if( !padDataFile.open(QFile::ReadOnly)){
            QMessageBox::warning(this,"Error",tr("打开校检数据文件失败:")+QFileInfo(padDataFile).fileName());
            return;
        }
        log(QFileInfo(padDataFile).fileName()+"...");
        currentLine=0;
        while( true )
        {
            line = QString(padDataFile.readLine(200));
            if( line.length() <= 1) break;
            currentLine++;
            //ignore the title
            if( line.left(6) == "Device") continue;

            if( !mDataStorer.MsgToPadData(line,paddata) ){
                if( QMessageBox::Yes == QMessageBox::warning(this,"Warn",tr("是否忽略  ")+QFileInfo(padDataFile).fileName()+tr("解释错误:行")+QString::number(currentLine),QMessageBox::Yes,QMessageBox::No))
                {
                     continue;
                }else{
                    padDataFile.close();
                     return;
                }
            }

            if( !gapExcelMap.contains(paddata.barcode) ){
                excelLinenumber++;
                gapExcelMap.insert(paddata.barcode,excelLinenumber);
                if( ! mExcel.SetCellData(excelLinenumber, 1, paddata.barcode) ||
                        !mExcel.SetCellData(excelLinenumber, 2, paddata.mac) ||
                        !mExcel.SetCellData(excelLinenumber, 3, paddata.temp_comp) ||
                        !mExcel.SetCellData(excelLinenumber, 4, paddata.Cali_ADC) ||
                        !mExcel.SetCellData(excelLinenumber, 5, paddata.Loadcellzero) ||
                        !mExcel.SetCellData(excelLinenumber, 6, paddata.UID)){
                    QMessageBox::warning(this,"Error",tr("Excel添加校检数据失败"));
                    padDataFile.close();
                    return;
                }
            }else{

                qint64 row = gapExcelMap.value(paddata.barcode);
                if( ! mExcel.GetCellData(row,2).isNull() ){
                    if( QMessageBox::No ==  QMessageBox::warning(this,"Error",QFileInfo(padDataFile).fileName()+":\n"+paddata.barcode+tr("序列号重复,行")+QString::number(currentLine)+tr("序列号重复,覆盖？"), QMessageBox::Yes,QMessageBox::No))
                    {
                        log(tr("合并数据失败，退出"));
                        return;
                    }
                    //recovery
                }
                if( !mExcel.SetCellData(row, 2, paddata.mac) ||
                        !mExcel.SetCellData(row, 3, paddata.temp_comp) ||
                        !mExcel.SetCellData(row, 4, paddata.Cali_ADC) ||
                        !mExcel.SetCellData(row, 5, paddata.Loadcellzero) ||
                        !mExcel.SetCellData(row, 6, paddata.UID)){
                    QMessageBox::warning(this,"Error",tr("Excel写入校检数据失败"));
                    padDataFile.close();
                    return;
                }
            }
        }
        padDataFile.close();
    }

    log(tr("保存文件：")+mExcel.getFilePath());

    mExcel.Save();
    mExcel.Close();

    log(tr("完成，退出Excel"));

}


















