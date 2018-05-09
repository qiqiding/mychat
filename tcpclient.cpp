#include "tcpclient.h"
#include "ui_tcpclient.h"
#include<QMessageBox>
#include<QHostAddress>
TcpClient::TcpClient(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TcpClient)
{
    ui->setupUi(this);
    ui->tcpClientStatusLabel->hide();
    //设置窗口有最大最小化按钮
    Qt::WindowFlags flags=Qt::Dialog;
    flags |=Qt::WindowMinMaxButtonsHint;
    flags |=Qt::WindowCloseButtonHint;
    setWindowFlags(flags);
    setWindowTitle(tr("接收文件"));
    TotalBytes = 0;
    bytesReceived = 0;
    fileNameSize = 0;
    tcpClientSocket = new QTcpSocket(this);
    tcpPort = 6666;
    connect(tcpClientSocket,&QTcpSocket::readyRead,this,&TcpClient::readMessage);
    connect(tcpClientSocket,SIGNAL(error(QAbstractSocket::SocketError)), this,SLOT(displayError(QAbstractSocket::SocketError)));

}

TcpClient::~TcpClient()
{
    delete ui;
}

// 设置文件名
void TcpClient::setFileName(QString fileName)
{
    localFile = new QFile(fileName);
}

// 获取发送端IP地址
void TcpClient::setHostAddress(QHostAddress address)
{

    hostAddress = address;
    newConnect();   
}

// 与服务器进行连接
void TcpClient::newConnect()
{

    blockSize = 0;
    tcpClientSocket->abort();        //取消已经存在的连接，并且重置套接字    
    tcpClientSocket->connectToHost(hostAddress, tcpPort);
    time.start();

}

// 读取数据
void TcpClient::readMessage()
{
    QDataStream in(tcpClientSocket);    //接收到数据直接流入此套接字
    in.setVersion(QDataStream::Qt_4_7);
    float useTime = time.elapsed();
    if (bytesReceived <= sizeof(qint64)*2) {    //说明刚开始接受数据
        if ((tcpClientSocket->bytesAvailable()>= sizeof(qint64)*2) && (fileNameSize == 0))
        {
             //bytesAvailable为返回将要被读取的字节数
            //接受数据总大小信息和文件名大小信息
            in>>TotalBytes>>fileNameSize;
            bytesReceived += sizeof(qint64)*2;
        }
        if((tcpClientSocket->bytesAvailable() >= fileNameSize) && (fileNameSize != 0)){
            //开始接受文件，并建立文件
            in>>fileName;
            bytesReceived +=fileNameSize;
            if(!localFile->open(QFile::WriteOnly)){
                QMessageBox::warning(this,tr("应用程序"),tr("无法读取文件 %1:\n%2.")
                                     .arg(fileName).arg(localFile->errorString()));
                return;
            }
        } else {
            return;
        }
    }
    if (bytesReceived < TotalBytes) {
        bytesReceived += tcpClientSocket->bytesAvailable();//返回tcpClient中字节的总数
        inBlock = tcpClientSocket->readAll();    //返回读到的所有数据
        localFile->write(inBlock);
        inBlock.resize(0);
    }
    ui->progressBar->setMaximum(TotalBytes);
    ui->progressBar->setValue(bytesReceived);
    double speed = bytesReceived / useTime;
    ui->tcpClientStatusLabel->setText(tr("已接收 %1MB (%2MB/s) "
                                         "\n共%3MB 已用时:%4秒\n估计剩余时间：%5秒")
                                      .arg(bytesReceived / (1024*1024))
                                      .arg(speed*1000/(1024*1024),0,'f',2)
                                      .arg(TotalBytes / (1024 * 1024))
                                      .arg(useTime/1000,0,'f',0)
                                      .arg(TotalBytes/speed/1000 - useTime/1000,0,'f',0));
    if(bytesReceived == TotalBytes)
    {
        localFile->close();
        tcpClientSocket->close();
        ui->tcpClientStatusLabel->setText(tr("接收文件 %1 完毕")
                                          .arg(fileName));
    }
}

//错误处理：QAbstractSocket类提供了所有scoket的通用功能，socketError为枚举型
void TcpClient::displayError(QAbstractSocket::SocketError socketError)
{
    switch(socketError)
    {   
    case QAbstractSocket::RemoteHostClosedError : break;//RemoteHostClosedError为远处主机关闭了连接时发出的错误信号
    default : qDebug() << tcpClientSocket->errorString();
    }
}

//取消按钮
void TcpClient::on_tcpClientCancleBtn_clicked()
{
    tcpClientSocket->abort();//取消当前连接
        if (localFile->isOpen())
            localFile->close();
        this->close();//关闭本对话框
}

// 关闭事件
void TcpClient::closeEvent(QCloseEvent *)
{
    on_tcpClientCancleBtn_clicked();
}

void TcpClient::on_pushButton_toggled(bool checked)//详细信息
{
    ui->tcpClientStatusLabel->setVisible(checked);
    if(checked)
        ui->pushButton->setText(tr("隐藏信息"));
    else
        ui->pushButton->setText(tr("详细信息"));
}
