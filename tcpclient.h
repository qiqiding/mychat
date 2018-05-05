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
    void setHostAddress(QHostAddress address);
        void setFileName(QString fileName);
protected:
    void closeEvent(QCloseEvent *);

private slots:
    void on_tcpClientCancleBtn_clicked();

    void on_tcpClientCloseBtn_clicked();

    void newConnect();
        void readMessage();
        void displayError(QAbstractSocket::SocketError);

private:
    Ui::TcpClient *ui;
    QTcpSocket *tcpClient;
        quint16 blockSize;
        QHostAddress hostAddress;
        qint16 tcpPort;

        qint64 TotalBytes;//发送数据的总大小
        qint64 bytesReceived;//已经发送数据的大小
        qint64 bytesToReceive;//剩余数据大小
        qint64 fileNameSize;//文件大小
        QString fileName;//文件名
        QFile *localFile;
        QByteArray inBlock;//数据缓冲区，即存放每次要发送的数据块

        QTime time;

};

#endif // TCPCLIENT_H
