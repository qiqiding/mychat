#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QDialog>
#include <QTime>
class QFile;
class QTcpServer;
class QTcpSocket;

namespace Ui {
class TcpServer;
}

class TcpServer : public QDialog
{
    Q_OBJECT

public:
    explicit TcpServer(QWidget *parent = 0);
    ~TcpServer();

    void initServer();//初始化服务器
    void refused();//关闭服务器
protected:
    void closeEvent(QCloseEvent *);

private slots:
    void on_serverOpenBtn_clicked();//打开

    void on_serverSendBtn_clicked();

    void on_serverCloseBtn_clicked();

    void sendMessage();//发送数据

    void updateClientProgress(qint64 numBytes);//更新进度条

    void on_pushButton_toggled(bool checked);

    void on_pushButton_2_clicked();

signals:
    void sendFileName(QString fileName);

private:
    Ui::TcpServer *ui;
    qint16 tcpPort;
    QTcpServer *tcpServer;
    QString fileName;
    QString theFileName;
    QFile *localFile;//待发送的文件

    qint64 TotalBytes;//总共需要发送的字节数
    qint64 bytesWritten;//已发送字节数
    qint64 bytesToWrite;//待发送字节数
    qint64 payloadSize;//被初始化的一个常量
    QByteArray outBlock;

    QTcpSocket *clientConnection;//客户端连接的套接字

    QTime time;
};

#endif // TCPSERVER_H
