#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QSettings>
#include <QFile>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    log(QString str);
    clearLog();
    connectServer();
    disconnectServer();
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
        ERROR_DATA
    };
public slots:
    socket_Disconnected();
    socket_Connected();
    socket_state(QAbstractSocket::SocketState socketState);
    socket_Read_Data();
private slots:
    void on_ConnectButton_clicked();

    void on_sendButton_clicked();

    void on_resetInputpushButton_clicked();

    void on_reWritepushButton_clicked();

    void on_saveSettingpushButton_clicked();

    void on_idLineEdit_textChanged(const QString &arg1);

    void on_ipLineEdit_textChanged(const QString &arg1);

    void on_portLineEdit_textChanged(const QString &arg1);

    void on_saveDataFilelineEdit_textChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
    QTcpSocket *mSocket;
    int mStep;
    bool mPackgetReplied;
    void syncLoopReset();
    void syncLoop(QByteArray data);

    int mInputIndex;
    QSettings mSetting;
    QFile dataFile;

    disableNetworkSetting(bool disable);
    updateDataInputState();
    disableDataInputState();
    void sendDataMessage();
protected:
    virtual void keyReleaseEvent(QKeyEvent *ev);
};

#endif // MAINWINDOW_H
