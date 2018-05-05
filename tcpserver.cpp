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

    setFixedSize(350,180);    //初始化时窗口显示固定大小

        tcpPort = 6666;        //tcp通信端口
        tcpServer = new QTcpServer(this);
        //newConnection表示当tcp有新连接时就发送信号
        connect(tcpServer, SIGNAL(newConnection()), this, SLOT(sendMessage()));

        initServer();

}

TcpServer::~TcpServer()
{
    delete ui;
}
// 初始化
void TcpServer::initServer()
{
    payloadSize = 64*1024;
    TotalBytes = 0;
    bytesWritten = 0;
    bytesToWrite = 0;

    ui->serverStatusLabel->setText(tr("请选择要传送的文件"));
    ui->progressBar->reset();//进度条复位
    ui->serverOpenBtn->setEnabled(true);//open按钮可用
    ui->serverSendBtn->setEnabled(false);//发送按钮不可用

    tcpServer->close();//tcp传送文件窗口不显示
}

// 开始发送数据
void TcpServer::sendMessage()    //是connect中的槽函数
{
    ui->serverSendBtn->setEnabled(false);    //当在传送文件的过程中，发送按钮不可用
    clientConnection = tcpServer->nextPendingConnection();    //用来获取一个已连接的TcpSocket
    //bytesWritten为qint64类型，即长整型
    connect(clientConnection, SIGNAL(bytesWritten(qint64)),    //?
            this, SLOT(updateClientProgress(qint64)));

    ui->serverStatusLabel->setText(tr("开始传送文件 %1 ！").arg(theFileName));

    localFile = new QFile(fileName);    //localFile代表的是文件内容本身
    if(!localFile->open((QFile::ReadOnly))){
        QMessageBox::warning(this, tr("应用程序"), tr("无法读取文件 %1:\n%2")
                             .arg(fileName).arg(localFile->errorString()));//errorString是系统自带的信息
        return;
    }
    TotalBytes = localFile->size();//文件总大小
    //头文件中的定义QByteArray outBlock;
    QDataStream sendOut(&outBlock, QIODevice::WriteOnly);//设置输出流属性
    sendOut.setVersion(QDataStream::Qt_4_7);//设置Qt版本，不同版本的数据流格式不同
    time.start();  // 开始计时
    QString currentFile = fileName.right(fileName.size()    //currentFile代表所选文件的文件名
                                         - fileName.lastIndexOf('/')-1);
    //qint64(0)表示将0转换成qint64类型,与(qint64)0等价
    //如果是，则此处为依次写入总大小信息空间，文件名大小信息空间，文件名
    sendOut << qint64(0) << qint64(0) << currentFile;
    TotalBytes += outBlock.size();//文件名大小等信息+实际文件大小
    //sendOut.device()为返回io设备的当前设置，seek(0)表示设置当前pos为0
    sendOut.device()->seek(0);//返回到outBlock的开始，执行覆盖操作
    //发送总大小空间和文件名大小空间
    sendOut << TotalBytes << qint64((outBlock.size() - sizeof(qint64)*2));
    //qint64 bytesWritten;bytesToWrite表示还剩下的没发送完的数据
    //clientConnection->write(outBlock)为套接字将内容发送出去，返回实际发送出去的字节数
    bytesToWrite = TotalBytes - clientConnection->write(outBlock);
    outBlock.resize(0);//why??
}

// 更新进度条，有数据发送时触发
void TcpServer::updateClientProgress(qint64 numBytes)
{
    //qApp为指向一个应用对象的全局指针
    qApp->processEvents();//processEvents为处理所有的事件？
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

void TcpServer::on_serverOpenBtn_clicked()
{
    //QString fileName;QFileDialog是一个提供给用户选择文件或目录的对话框
        fileName = QFileDialog::getOpenFileName(this);    //filename为所选择的文件名(包含了路径名)
        if(!fileName.isEmpty())
        {
            //fileName.right为返回filename最右边参数大小个字文件名，theFileName为所选真正的文件名
            theFileName = fileName.right(fileName.size() - fileName.lastIndexOf('/')-1);
            ui->serverStatusLabel->setText(tr("要传送的文件为：%1 ").arg(theFileName));
            ui->serverSendBtn->setEnabled(true);//发送按钮可用
            ui->serverOpenBtn->setEnabled(false);//open按钮禁用
        }

}

void TcpServer::on_serverSendBtn_clicked()
{
    //tcpServer->listen函数如果监听到有连接，则返回1，否则返回0
        if(!tcpServer->listen(QHostAddress::Any,tcpPort))//开始监听6666端口
        {
            qDebug() << tcpServer->errorString();//此处的errorString是指？
            close();
            return;
        }

        ui->serverStatusLabel->setText(tr("等待对方接收... ..."));
        emit sendFileName(theFileName);//发送已传送文件的信号，在widget.cpp构造函数中的connect()触发槽函数

}

void TcpServer::on_serverCloseBtn_clicked()
{
    if(tcpServer->isListening())
        {
            //当tcp正在监听时，关闭tcp服务器端应用，即按下close键时就不监听tcp请求了
            tcpServer->close();
            if (localFile->isOpen())//如果所选择的文件已经打开，则关闭掉
                localFile->close();
            clientConnection->abort();//clientConnection为下一个连接？怎么理解
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
