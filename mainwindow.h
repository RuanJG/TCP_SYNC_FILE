#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpServer>
#include <QTcpSocket>
#include <tcpserverworker.h>
#include <QMutex>
#include <QSettings>
#include <datastorer.h>
#include <excelengine.h>
#include <serialcoder.h>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    typedef enum _PAD_MSG_TAG{
        MD5_CODE_TAG = -1,
        FILE_SIZE_TAG = -2,
        // tag >=0  is the file pos
    }PAD_MSG_TAG;

    log(QString str);
    clearLog();
    stop_server_listen();
    bool saveMsgToFile(QString file, QString msg);
    QString getPadDataFilePath(QString clientname);
    bool createDir(const QString &path);
    QString getPadDataBackupFilePath(QString clientname);
    bool pad_tester_data_store(TcpServerWorker *worker, qint64 tag, QByteArray msg);
    void mergeData();
public slots:
    slot_worker_socket_abort(TcpServerWorker *worker);
    slot_worker_data_received(TcpServerWorker *worker, QByteArray buffer);
private slots:
    new_Client_Connect();
    void on_serverOnOFFButton_clicked();

    void on_mergepushButton_clicked();

private:
    Ui::MainWindow *ui;
    QTcpServer *mServer;
    QList<TcpServerWorker *> mClientList;
    QMutex mPCDataMute;
    QMutex mPADDataMute;
    QSettings mSetting;
    DataStorer mDataStorer;
    QString mPCMsgBackupfile ;
    QString mPADMsgBackupfile;
    QString mPC_TC_MsgBackupfile ;
    QString mPC_Adjusted_MsgBackupfile ;
    DataStorer mTCDataStorer;
    DataStorer mAdjustedDataStorer;
    ExcelEngine mExcel;
    bool syncSuccessed;
    qint64 mPadFileSize;
    bool start_server_listen();
    server_setup_worker_id_type(TcpServerWorker *worker, QByteArray buffer);
    bool pc_tester_data_check_and_store(TcpServerWorker *worker,QString id, QByteArray data);
    bool pad_tester_data_check_and_store(TcpServerWorker *worker,QString id, QByteArray data);
};

#endif // MAINWINDOW_H
