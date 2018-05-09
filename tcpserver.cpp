#include "tcpserver.h"
#include "ui_tcpserver.h"
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>

TcpServer::TcpServer(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TcpServer)
{
    ui->setupUi(this);
    setWindowTitle(tr("发送文件"));
    ui->serverStatusLabel->hide();//设置该标签隐藏
    //设置窗口有最大最小化按钮
    Qt::WindowFlags flags=Qt::Dialog;
    flags |=Qt::WindowMinMaxButtonsHint;
    flags |=Qt::WindowCloseButtonHint;
    setWindowFlags(flags);
    tcpPort = 6666;        //tcp通信端口
    tcpServer = new QTcpServer(this);
    connect(tcpServer,&QTcpServer::newConnection,this,&TcpServer::sendMessage);//一旦有客户端连接到服务器，则发射newConnection信号
    initServer();
}

TcpServer::~TcpServer()
{
    delete ui;
}
// 初始化
void TcpServer::initServer()
{
    payloadSize = 64*1024;//64KB
    TotalBytes = 0;
    bytesWritten = 0;
    bytesToWrite = 0;    
    ui->progressBar->reset();//进度条复位
    ui->serverOpenBtn->setEnabled(true);//open按钮可用
    ui->serverSendBtn->setEnabled(false);//发送按钮不可用
    tcpServer->close();//关闭服务器
}

// 开始发送数据
void TcpServer::sendMessage()    //是connect中的槽函数
{   
    ui->serverSendBtn->setEnabled(false);    //当在传送文件的过程中，发送按钮不可用
    clientConnection = tcpServer->nextPendingConnection();    //用来获取一个已连接的TcpSocket  
    connect(clientConnection, SIGNAL(bytesWritten(qint64)),this, SLOT(updateClientProgress(qint64)));
    ui->serverStatusLabel->setText(tr("开始传送文件 %1 ！").arg(theFileName));
    localFile = new QFile(fileName);    //localFile代表的是文件内容本身
    if(!localFile->open((QFile::ReadOnly)))//以只读的方式打开文件
    {
        QMessageBox::warning(this, tr("应用程序"), tr("无法读取文件 %1:\n%2").arg(fileName).arg(localFile->errorString()));//errorString是系统自带的信息
        return;
    }
    TotalBytes = localFile->size();//获取文件总大小
    QDataStream sendOut(&outBlock, QIODevice::WriteOnly);//设置输出流属性，将发送缓冲区outblock封装在一个QDataStream
    sendOut.setVersion(QDataStream::Qt_4_7);//设置Qt版本，不同版本的数据流格式不同
    time.start();  // 开始计时
    QString currentFile = fileName.right(fileName.size()-fileName.lastIndexOf('/')-1);//通过QString类的right()函数去掉文件的路径部分
    //仅仅将文件部分保存在currentFile
    //如果是，则此处为依次写入总大小信息空间（先占位），文件名大小信息空间，文件名
    sendOut << qint64(0) << qint64(0) << currentFile;
    TotalBytes += outBlock.size();//文件名大小等信息+实际文件大小
    sendOut.device()->seek(0);//将读写操作指向从头开始
    sendOut << TotalBytes << qint64((outBlock.size()-sizeof(qint64)*2));//填写实际的总长度和文件长度
    //qint64 bytesWritten;bytesToWrite表示还剩下的没发送完的数据
    //clientConnection->write(outBlock)为套接字将内容发送出去，返回实际发送出去的字节数
    bytesToWrite = TotalBytes-clientConnection->write(outBlock);//将该头文件发出，同时修改待发送字节数bytesToWrite
    outBlock.resize(0);//清空缓存区以备下次使用
}

// 更新进度条，有数据发送时触发
void TcpServer::updateClientProgress(qint64 numBytes)
{
    qApp->processEvents();//用于传输大文件时使界面不会冻结
    bytesWritten += (int)numBytes;
    if (bytesToWrite > 0) {    //没发送完毕
        //初始化时payloadSize = 64*1024;qMin为返回参数中较小的值，每次最多发送64K的大小
        outBlock = localFile->read(qMin(bytesToWrite, payloadSize));
        bytesToWrite -= (int)clientConnection->write(outBlock);
        outBlock.resize(0);//清空发送缓冲区
    } else {
        localFile->close();
    }
    ui->progressBar->setMaximum(TotalBytes);//进度条的最大值为所发送信息的所有长度(包括附加信息)
    ui->progressBar->setValue(bytesWritten);//进度条显示的进度长度为bytesWritten实时的长度
    float useTime = time.elapsed();//从time.start()还是到当前所用的时间记录在useTime中
    double speed = bytesWritten / useTime;
    ui->serverStatusLabel->setText(tr("已发送 %1MB (%2MB/s) "
                   "\n共%3MB 已用时:%4秒\n估计剩余时间：%5秒")
                   .arg(bytesWritten / (1024*1024))    //转化成MB
                   .arg(speed*1000 / (1024*1024), 0, 'f', 2)
                   .arg(TotalBytes / (1024 * 1024))
                   .arg(useTime/1000, 0, 'f', 0)    //0，‘f’,0是什么意思啊？
                   .arg(TotalBytes/speed/1000 - useTime/1000, 0, 'f', 0));
    if(bytesWritten == TotalBytes) {    //当需发送文件的总长度等于已发送长度时，表示发送完毕！
        localFile->close();
        tcpServer->close();
        ui->serverStatusLabel->setText(tr("传送文件 %1 成功").arg(theFileName));
    }
}

//打开按钮
void TcpServer::on_serverOpenBtn_clicked()
{
    //QString fileName;QFileDialog是一个提供给用户选择文件或目录的对话框
        fileName = QFileDialog::getOpenFileName(this);    //filename为所选择的文件名(包含了路径名)
        if(!fileName.isEmpty())
        {
            //fileName.right为返回filename最右边参数大小个字文件名，theFileName为所选真正的文件名
            theFileName = fileName.right(fileName.size()-fileName.lastIndexOf('/')-1);
            ui->serverStatusLabel->setText(tr("要传送的文件为：%1 ").arg(theFileName));
            ui->serverSendBtn->setEnabled(true);//发送按钮可用
            ui->serverOpenBtn->setEnabled(false);//open按钮禁用
        }

}
//发送按钮（开始监听）
void TcpServer::on_serverSendBtn_clicked()
{
    //单击“发送”按钮后，将服务器设置为监听状态，然后发送sendFileName()信号，在主界面类中将关联该信号并使用UDP广播将文件名发送给接收端
    //tcpServer->listen函数如果监听到有连接，则返回1，否则返回0
        if(!tcpServer->listen(QHostAddress::Any,tcpPort))//开始监听任何连在6666端口的IP地址
        {
            qDebug() << tcpServer->errorString();
            close();           
        }       
        emit sendFileName(theFileName);//发送已传送文件的信号，在widget.cpp构造函数中的connect()触发槽函数
}

//关闭按钮，先关闭服务器再关闭对话框
void TcpServer::on_serverCloseBtn_clicked()
{
    if(tcpServer->isListening())//当tcp正在监听时，关闭tcp服务器端应用，即按下close键时就不监听tcp请求了
        {      
            tcpServer->close();
            if (localFile->isOpen())//如果所选择的文件已经打开，则关闭掉
                localFile->close();
            clientConnection->abort();//clientConnection关掉
        }
        close();//关闭本ui，即本对话框
}

// 被对方拒绝
void TcpServer::refused()
{
    tcpServer->close();
    ui->serverStatusLabel->setText(tr("对方拒绝接收！！！"));
}

// 关闭事件
void TcpServer::closeEvent(QCloseEvent *)
{
    on_serverCloseBtn_clicked();
}

void TcpServer::on_pushButton_toggled(bool checked)//显示详细信息
{
    ui->serverStatusLabel->setVisible(checked);
    if(checked)
        ui->pushButton->setText(tr("隐藏信息"));
    else
        ui->pushButton->setText(tr("详细信息"));
}
