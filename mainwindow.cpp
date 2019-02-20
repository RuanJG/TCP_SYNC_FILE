#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QHostAddress>
#include <QFile>
#include <QMessageBox>
#include <QKeyEvent>
#include <QDate>
#include <QDateTime>
#include <QInputDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    mSocket(NULL),
    mPackgetReplied(false),
    mSetting(qApp->applicationDirPath()+"\/Setting.ini",QSettings::IniFormat),
    mOneDataFile(true),
    mLastMsg(),
    mSavefilenameChangeAble(false)
{
    ui->setupUi(this);
    ui->ConnectButton->setText(tr("连接服务器"));
    mStep = 0;

    ui->tabWidget->setCurrentIndex(1);

    disableDataInputState();

    if( mSetting.contains("net/ip"))
        ui->ipLineEdit->setText(mSetting.value("net/ip").toString());
    if( mSetting.contains("net/port"))
        ui->portLineEdit->setText(mSetting.value("net/port").toString());
    if( mSetting.contains("net/id"))
        ui->idLineEdit->setText(mSetting.value("net/id").toString());
    if( mSetting.contains("ui/TemptureCompensation") ){
        if (mSetting.value("ui/TemptureCompensation").toString() == "Yes"){
            ui->GapafterTemptureCailradioButton_2->setChecked(true);
            ui->gapTypelabel_8->setText(tr("录入温保后数据"));
        }else if(mSetting.value("ui/TemptureCompensation").toString() == "No" ){
            ui->GapafterTemptureCailradioButton_2->setChecked(true);
            ui->gapTypelabel_8->setText(tr("录入温保前数据"));
        }else if( mSetting.value("ui/TemptureCompensation").toString() == "Adjust"  ){
            ui->adjustRadioButton->setChecked(true);
            ui->gapTypelabel_8->setText(tr("录入调校后数据"));
        }else{
            ui->GapafterTemptureCailradioButton_2->setChecked(true);
            ui->gapTypelabel_8->setText(tr("录入温保后数据"));
        }
    }
    if(mSetting.contains("parameter/gapmin")){
        ui->minGaplineEdit->setText(mSetting.value("parameter/gapmin").toString());
    }else{
        ui->minGaplineEdit->setText("6.45");
        mSetting.setValue("parameter/gapmin","6.45");
        mSetting.sync();
    }
    if(mSetting.contains("parameter/gapmax")){
        ui->maxGaplineEdit_2->setText(mSetting.value("parameter/gapmax").toString());
    }else{
        ui->maxGaplineEdit_2->setText("6.55");
        mSetting.setValue("parameter/gapmax","6.55");
        mSetting.sync();
    }
    ui->gapTypelabel_8->setStyleSheet("color:blue;");
    on_ConnectButton_clicked();

    if( mOneDataFile )
    {
        if(mSavefilenameChangeAble && mSetting.contains("data/savefile")){
            //QString file = mSetting.value("data/savefile").toString();
            ui->saveDataFilelineEdit->setText(mSetting.value("data/savefile").toString());
        }else{
            //QString file2 = mSetting.value("data/savefile").toString();
            QString tc;
            if( ui->gapBeforTempCailradioButton->isChecked() ) tc = tr("温保前");
            if( ui->GapafterTemptureCailradioButton_2->isChecked() ) tc = tr("温保后");
            if( ui->adjustRadioButton->isChecked() ) tc = tr("调整后");
            QString file = qApp->applicationDirPath()+"\/SBSdata"+tc+".txt";
            ui->saveDataFilelineEdit->setText(file);
        }
        ui->saveDataFilelineEdit->setReadOnly(true);
    }else{
        QDate date = QDate::currentDate();
        QString file = qApp->applicationDirPath()+"\/SBSdata_"+date.toString("yyyyMMdd")+".txt";
        ui->saveDataFilelineEdit->setText(file);
        ui->saveDataFilelineEdit->setReadOnly(true);
    }

    ui->barcodelineEdit->setValidator(new QRegExpValidator(QRegExp("[A-F0-9]{16}"),this));
    ui->leftlineEdit->setValidator(new QRegExpValidator(QRegExp("[.0-9]{4}"),this));
    ui->rightlineEdit->setValidator(new QRegExpValidator(QRegExp("[.0-9]{4}"),this));
    connect(ui->barcodelineEdit,SIGNAL(returnPressed()),this, SLOT(slot_barcodeEdit_get_Return_KEY()));
    connect(ui->leftlineEdit,SIGNAL(returnPressed()),this, SLOT(slot_leftedit_get_Return_KEY()));
    connect(ui->rightlineEdit,SIGNAL(returnPressed()),this, SLOT(slot_rightedit_get_Return_KEY()));

    ui->datatextBrowser->append(tr("时间----------  序列号---------- 左间隙-右间隙"));
}


MainWindow::~MainWindow()
{
    delete ui;
}


MainWindow::log(QString str)
{
    ui->console->append(str);
}
MainWindow::clearLog()
{
    ui->console->clear();
}


MainWindow::disableNetworkSetting(bool disable)
{
    ui->ipLineEdit->setDisabled(disable);
    ui->portLineEdit->setDisabled(disable);
    ui->idLineEdit->setDisabled(disable);
}

MainWindow::disableDataInputState()
{
    ui->barcodelineEdit->setText("");
    ui->leftlineEdit->setText("");
    ui->rightlineEdit->setText("");
    ui->barcodelineEdit->setEnabled(false);
    ui->leftlineEdit->setEnabled(false);
    ui->rightlineEdit->setEnabled(false);

    mInputIndex = -1;
}

MainWindow::updateDataInputState()
{
    mInputIndex++;
    mInputIndex %= 4;

    switch(mInputIndex)
    {
    case 0:
        ui->barcodelineEdit->setEnabled(true);
        ui->barcodelineEdit->setFocus();
        ui->leftlineEdit->setEnabled(false);
        ui->rightlineEdit->setEnabled(false);
        ui->stateLabel->setText(tr("输入序列号,按Enter结束"));
        break;
    case 1:
        ui->barcodelineEdit->setEnabled(false);
        ui->leftlineEdit->setEnabled(true);
        ui->leftlineEdit->setFocus();
        ui->rightlineEdit->setEnabled(false);
        ui->stateLabel->setText(tr("输入左间隙,按Enter结束"));
        break;
    case 2:
        ui->barcodelineEdit->setEnabled(false);
        ui->leftlineEdit->setEnabled(false);
        ui->rightlineEdit->setEnabled(true);
        ui->rightlineEdit->setFocus();
        ui->stateLabel->setText(tr("输入右间隙,按Enter结束"));
        break;
    case 3:
        ui->barcodelineEdit->setEnabled(false);
        ui->leftlineEdit->setEnabled(false);
        ui->rightlineEdit->setEnabled(false);
        ui->stateLabel->setText(tr("发送数据..."));
        int res = QMessageBox::warning(this,tr("发送数据"),
                             QString("序列号 "+ui->barcodelineEdit->text()+"\n左间隙 "+ui->leftlineEdit->text()+"\n右间隙 "+ui->rightlineEdit->text()),
                             QMessageBox::Yes,QMessageBox::No);

        if( res == QMessageBox::Yes ){
            sendDataMessage();
        }else{
            mInputIndex =4;
            ui->barcodelineEdit->setEnabled(true);
            ui->barcodelineEdit->setFocus();
            ui->leftlineEdit->setEnabled(false);
            ui->rightlineEdit->setEnabled(false);
            ui->stateLabel->setText(tr("输入序列号,按Enter结束"));
        }

        break;

    }
}

MainWindow::socket_Read_Data()
{
    QByteArray buffer;

    buffer = mSocket->readAll();
    log("Server send :"+ QString::fromLocal8Bit(buffer));
    syncLoop( buffer );
}

MainWindow::socket_Disconnected()
{
    log("Disconnected OK");
    ui->ConnectButton->setText(tr("连接服务器"));
    ui->ConnectButton->setEnabled(true);
    syncLoopReset();
    disableNetworkSetting(false);
    ui->stateLabel->setStyleSheet("color:red;");
    ui->stateLabel->setText(tr("未连接服务器"));
    disableDataInputState();
}
MainWindow::socket_Connected()
{
    ui->ConnectButton->setDisabled(false);
    log("connected OK");
    ui->ConnectButton->setText(tr("断开服务器"));

    syncLoop(QByteArray());
    //disableNetworkSetting(true);
    ui->stateLabel->setStyleSheet("color:blue;");
    ui->stateLabel->setText(tr("已连接服务器"));
    updateDataInputState();
}

MainWindow::socket_state(QAbstractSocket::SocketState socketState)
{
    log("socket status change: "+  socketState);
    switch(socketState)
    {
    case QAbstractSocket::UnconnectedState:
        log("UnconnectedState");
        socket_Disconnected();
        break;
    case QAbstractSocket::HostLookupState:
        log("HostLookupState");
        break;
    case QAbstractSocket::ConnectingState:
        log("ConnectingState");
        break;
    case QAbstractSocket::ConnectedState:
        log("ConnectedState");
        socket_Connected();
        break;
    case QAbstractSocket::BoundState:
        log("BoundState");
        break;
    case QAbstractSocket::ClosingState:
        log("ClosingState");
        break;
    case QAbstractSocket::ListeningState:
        log("ListeningState");
        break;
    }
}

MainWindow::connectServer()
{
    disableNetworkSetting(true);

    QString ip=ui->ipLineEdit->text();
    int port = ui->portLineEdit->text().toInt();

    disconnectServer();
    mSocket = new QTcpSocket();
    connect(mSocket, &QTcpSocket::readyRead, this, socket_Read_Data);
    //connect(mSocket, &QTcpSocket::disconnected, this, socket_Disconnected);
    //connect(mSocket, &QTcpSocket::connected, this, socket_Connected);
    connect(mSocket, &QTcpSocket::stateChanged, this, socket_state);

    QHostAddress addr = QHostAddress(ip);
    if( addr.toString() == ip){
        log("connect ip:"+ip);
        mSocket->connectToHost(QHostAddress(ip) , port);
    }else{
        log("connect hostname:"+ip);
        mSocket->connectToHost(ip,port);
    }

    log("connecting "+ip+":"+QString::number(port)+"...");
}

MainWindow::disconnectServer()
{
    if( mSocket != NULL ){
        mSocket->disconnectFromHost();
        disconnect(mSocket, &QTcpSocket::readyRead, this, socket_Read_Data);
        disconnect(mSocket, &QTcpSocket::disconnected, this, socket_Disconnected);
        disconnect(mSocket, &QTcpSocket::connected, this, socket_Connected);
        disconnect(mSocket, &QTcpSocket::stateChanged, this, socket_state);
        mSocket->abort();
        mSocket->deleteLater();
        mSocket = NULL;
    }
}

bool MainWindow::saveMsgToFile(QString msg)
{
    QFile savefile(ui->saveDataFilelineEdit->text());
    if( !savefile.open(QFile::Append|QFile::Text)){
        QMessageBox::warning(this,tr("错误"), tr("打开数据文件失败"));
        return false;
    }
    //savefile.seek(savefile.size());
    savefile.write(msg.toLocal8Bit());
    savefile.flush();

    ui->datatextBrowser->append(msg);

    savefile.close();
    return true;
}

void MainWindow::syncLoop(QByteArray data)
{
    if( mSocket == NULL ) return;

    int res ;
    QString idpack,typePack;



    if( data.length() == 4 && data.left(3) == "ACK" )
    {
        mPackgetReplied = true;
        res = QString(data.right(1)).toInt();
        //QMessageBox::warning(this,QString(data),"ack="+QString::number(res));
        switch (res)
        {
        case MainWindow::ACKType::OK:
            switch( mStep )
            {
            case 0:
                //send type
                log("send type...");
                ui->stateLabel->setText(tr("认证 TYPE..."));
                if(ui->gapBeforTempCailradioButton->isChecked()){
                    typePack = "TYPE"+QString::number(ClientType::PCTESTER);
                }else if( ui->GapafterTemptureCailradioButton_2->isChecked()){
                    //已做过温保
                    typePack = "TYPE"+QString::number(ClientType::PCTESTER_TC);
                }else if ( ui->adjustRadioButton->isChecked()){
                    typePack = "TYPE"+QString::number(ClientType::PCTESTER_ADJUST);
                }else{
                    QMessageBox::warning(this,tr("认证失败"),tr("请选择Gap数据类型（温保前后或调校"));
                    break ;
                }
                mSocket->write(typePack.toLocal8Bit());
                mStep++;
                break;
            case 1:
                ui->stateLabel->setStyleSheet("color:blue;");
                ui->stateLabel->setText(tr("已登录服务器"));
                mStep++;
                break;
            case 2:
                ui->stateLabel->setText(tr("发送数据成功"));
                saveMsgToFile(mLastMsg);
                disableDataInputState();
                updateDataInputState();
                break;
            default:
                QMessageBox::warning(this,tr("错误"), tr("到达未知步骤"),NULL,NULL);
                break;
            }
            break;
        case MainWindow::ACKType::ERROR_ID:
            QMessageBox::warning(this,tr("错误"), tr("ID重复，重新设置"),NULL,NULL);
            disconnectServer();
            break;
        case MainWindow::ACKType::ERROR_TYPE:
            QMessageBox::warning(this,tr("错误"), tr("登录认证错误"),NULL,NULL);
            disconnectServer();
            break;
        case MainWindow::ACKType::ERROR_DATA:
            QMessageBox::warning(this,tr("通讯错误"), tr("数据包错误"),NULL,NULL);
            break;
        case MainWindow::ACKType::ERROR_FILE:
            QMessageBox::warning(this,tr("服务器错误"), tr("文件读写出错"),NULL,NULL);
            break;
        case MainWindow::ACKType::ERROR_REPEAT_BARCODE:
            if( QMessageBox::Yes == QMessageBox::warning(this,tr("数据错误"), tr("产品序列号有重复，是否要覆盖旧数据？"),QMessageBox::Yes,QMessageBox::No))
            {
                //send the last msg again , mean that to replace the old record in the server.
                mSocket->write(mLastMsg.toLocal8Bit());
            }else{
                disableDataInputState();
                updateDataInputState();
            }
            break;
        }

    }else{
        if( data.length() != 0 ){
            QMessageBox::warning(this,tr("通讯错误"), tr("未知数据:"+data),NULL,NULL);
            return;
        }
        //start login
        if( mStep == 0){
            log("send id...");
            ui->stateLabel->setText(tr("认证 ID..."));
            idpack = "ID"+ui->idLineEdit->text();
            mSocket->write(idpack.toLocal8Bit());
        }
    }

}


void MainWindow::syncLoopReset()
{
    mStep = 0;
}

void MainWindow::on_ConnectButton_clicked()
{
    ui->ConnectButton->setDisabled(true);

    if( ui->ConnectButton->text() == tr("连接服务器")){
        ui->stateLabel->setStyleSheet("color:black;");
        ui->stateLabel->setText(tr("正在连接服务器..."));
        connectServer();
    }else{
        disconnectServer();
    }

}

void MainWindow::on_sendButton_clicked()
{

}

void MainWindow::sendDataMessage()
{
    QDateTime date = QDateTime::currentDateTime();
    //QString date = QDate::toString("YYMMDDHHMMSS");
    QString msg = date.toString("yyyyMMddhhmmss")+" "+ui->barcodelineEdit->text()+" "+ui->leftlineEdit->text()+" "+ui->rightlineEdit->text()+"\n";

    mLastMsg = msg;

    if(mSocket->state() == QAbstractSocket::ConnectedState){
        //if( mStep == 2 && mPackgetReplied ){
            log("send data...");
        //    mPackgetReplied = false;
            mSocket->write(msg.toLocal8Bit());
        //}else{
        //    QMessageBox::warning(this,tr("错误"), tr("上次信息还没收到ACK"),NULL,NULL);
        //}
    }else{
        QMessageBox::warning(this,tr("错误"), tr("发送数据失败，未连接网络"),NULL,NULL);
    }
}


void MainWindow::slot_barcodeEdit_get_Return_KEY()
{
    if( ui->barcodelineEdit->text().length() != 16){
        QMessageBox::warning(this,tr("错误"),tr("序列号长度不等于16位，请重新输入"));
        ui->barcodelineEdit->clear();
    }else{
        updateDataInputState();
    }
}

void MainWindow::slot_leftedit_get_Return_KEY()
{
    bool inttostring =false;
    float value = 0.0;
    float min = mSetting.value("parameter/gapmin").toFloat();
    float max = mSetting.value("parameter/gapmax").toFloat();
    //min = min-0.001;
    //max = max + 0.001;

    value = ui->leftlineEdit->text().toFloat(&inttostring);

    if(  !inttostring || ( ui->adjustRadioButton->isChecked() && (value< min ||value> max)) ){
        QMessageBox::warning(this,tr("错误"),tr("产品不良,数据不在范围[%1,%2]内").arg(min).arg(max),NULL,NULL);
        disableDataInputState();
        updateDataInputState();
    }else{
        updateDataInputState();
    }
}
void MainWindow::slot_rightedit_get_Return_KEY()
{
    bool inttostring =false;
    float value = 0.0;
    float min = mSetting.value("parameter/gapmin").toFloat();
    float max = mSetting.value("parameter/gapmax").toFloat();
    //min = min-0.001;
    //max = max + 0.001;

    value = ui->rightlineEdit->text().toFloat(&inttostring);
    if( !inttostring || ( ui->adjustRadioButton->isChecked() && (value< min ||value> max)) ){
        QMessageBox::warning(this,tr("错误"),tr("产品不良,数据不在范围[%1,%2]内").arg(min).arg(max),NULL,NULL);
        disableDataInputState();
        updateDataInputState();
    }else{
        updateDataInputState();
    }
}



void MainWindow::on_resetInputpushButton_clicked()
{
    disableDataInputState();
    updateDataInputState();
}

void MainWindow::on_reWritepushButton_clicked()
{
    switch(mInputIndex)
    {
    case 0:
        ui->barcodelineEdit->setText("");
        ui->barcodelineEdit->setFocus();
        break;
    case 1:
        ui->leftlineEdit->setText("");
        ui->leftlineEdit->setFocus();
        break;
    case 2:
        ui->rightlineEdit->setText("");
        ui->rightlineEdit->setFocus();
        break;
    }
}


void MainWindow::on_idLineEdit_textChanged(const QString &arg1)
{
    mSetting.setValue("net/id", arg1);
    mSetting.sync();
}

void MainWindow::on_ipLineEdit_textChanged(const QString &arg1)
{
    mSetting.setValue("net/ip",arg1);
    mSetting.sync();
}

void MainWindow::on_portLineEdit_textChanged(const QString &arg1)
{
    mSetting.setValue("net/port", arg1);
    mSetting.sync();
}

void MainWindow::on_saveDataFilelineEdit_textChanged(const QString &arg1)
{
    //on_saveSettingpushButton_clicked();
    mSetting.setValue("data/savefile",arg1);
    mSetting.sync();
}

void MainWindow::on_pushButton_clicked()
{
    if( ui->idLineEdit->isReadOnly()){
        bool ok;
        QString pwd = QInputDialog::getText(this,tr("认证"),tr("密码:"));
        if( pwd == "a123"){
            if( mSavefilenameChangeAble )
                ui->saveDataFilelineEdit->setReadOnly(false);
            ui->ipLineEdit->setReadOnly(false);
            ui->idLineEdit->setReadOnly(false);
            ui->portLineEdit->setReadOnly(false);
            ui->maxGaplineEdit_2->setReadOnly(false);
            ui->minGaplineEdit->setReadOnly(false);
            ui->pushButton->setText(tr("确定"));
            ui->gapBeforTempCailradioButton->setEnabled(true);
            ui->GapafterTemptureCailradioButton_2->setEnabled(true);
            ui->adjustRadioButton->setEnabled(true);
            disconnectServer();
        }

    }else{
        ui->saveDataFilelineEdit->setReadOnly(true);
        ui->ipLineEdit->setReadOnly(true);
        ui->idLineEdit->setReadOnly(true);
        ui->portLineEdit->setReadOnly(true);
        ui->minGaplineEdit->setReadOnly(true);
        ui->maxGaplineEdit_2->setReadOnly(true);
        ui->pushButton->setText(tr("修改设置"));
        ui->gapBeforTempCailradioButton->setEnabled(false);
        ui->GapafterTemptureCailradioButton_2->setEnabled(false);
        ui->adjustRadioButton->setEnabled(false);
        connectServer();
    }
}


void MainWindow::on_gapBeforTempCailradioButton_clicked()
{

    QString tc = tr("温保前");
    QString file = qApp->applicationDirPath()+"\/SBSdata"+tc+".txt";
    ui->saveDataFilelineEdit->setText(file);
    mSetting.setValue("ui/TemptureCompensation", "No");
    ui->gapTypelabel_8->setText(tr("录入温保前数据"));
}


void MainWindow::on_GapafterTemptureCailradioButton_2_clicked()
{

    QString tc = tr("温保后");
    QString file = qApp->applicationDirPath()+"\/SBSdata"+tc+".txt";
    ui->saveDataFilelineEdit->setText(file);
    mSetting.setValue("ui/TemptureCompensation", "Yes");
    ui->gapTypelabel_8->setText(tr("录入温保后数据"));
}

void MainWindow::on_adjustRadioButton_clicked()
{
    QString tc = tr("调校后");
    QString file = qApp->applicationDirPath()+"\/SBSdata"+tc+".txt";
    ui->saveDataFilelineEdit->setText(file);
    mSetting.setValue("ui/TemptureCompensation", "Adjust");
    ui->gapTypelabel_8->setText(tr("录入调校后数据"));
}




void MainWindow::on_minGaplineEdit_textChanged(const QString &arg1)
{
    mSetting.setValue("parameter/gapmin",arg1);
    mSetting.sync();
}

void MainWindow::on_maxGaplineEdit_2_textChanged(const QString &arg1)
{
    mSetting.setValue("parameter/gapmax",arg1);
    mSetting.sync();
}
