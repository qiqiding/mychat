#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QDialog>
#include <QHostAddress>
#include <QFile>
#include <QTime>
//class QTcpSocket;
#include<QTcpSocket>
namespace Ui {
class TcpClient;
}

class TcpClient : public QDialog
{
    Q_OBJECT

public:
    explicit TcpClient(QWidget *parent = 0);
    ~TcpClient();
    void setHostAddress(QHostAddress address);//获取发送端IP地址

    void setFileName(QString fileName);//获取文件保存路径

protected:
    void closeEvent(QCloseEvent *);

private slots:
    void on_tcpClientCancleBtn_clicked();

    void newConnect();//连接到服务器

    void readMessage();//读取文件数据

    void displayError(QAbstractSocket::SocketError socketError);//显示错误信息

    void on_pushButton_toggled(bool checked);

private:
    Ui::TcpClient *ui;
    QTcpSocket *tcpClientSocket;//客户端套接字类
    quint16 blockSize;
    QHostAddress hostAddress;//发送端IP地址
    qint16 tcpPort;
    qint64 TotalBytes;//需要接收的数据的总大小
    qint64 bytesReceived;//已经接收数据的大小
    qint64 bytesToReceive;//剩余数据大小
    qint64 fileNameSize;//文件大小
    QString fileName;//文件名
    QFile *localFile;//待接收的文件
    QByteArray inBlock;//数据缓冲区，即存放每次要发送的数据块
    QTime time;

};

#endif // TCPCLIENT_H
