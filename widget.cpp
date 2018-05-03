#include "widget.h"
#include "ui_widget.h"
#include<QByteArray>
#include<QHostInfo>
#include<QDataStream>
#include<QMessageBox>
#include<QScrollBar>

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    udpSocket=new QUdpSocket(this);
    port=45454;
    udpSocket->bind(port,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
    //绑定端口号，采用ShareAddress模式（可以允许其他的服务器连接到相同的地址和端口，特别是用在
    //多客户端监听同一个服务器端口时特别有效），ReuseAddressHint为QUdpSocke提供提示，
    //即在地址和端口已经被其他套接字绑定的情况下，也应该试着重新绑定
    connect(udpSocket,SIGNAL(readyRead()),this,SLOT(processPendingDatagrams()));
            //在接口里有信息时，立马处理
    sendMessage(NewParticipant);//打开此软件，就说明是新用户加入，所以发射新用户加入广播

}

Widget::~Widget()
{
    delete ui;
}
void Widget::sendMessage(MessageType type, QString serverAddress)
{
    QByteArray data;//字节数组
    QDataStream out(&data,QIODevice::WriteOnly);//QDataStream是将序列化的二进制数据送到io设备，
    //因为其属性为只写
    QString localHostName=QHostInfo::localHostName();
    QString address=getIP();//调用自己类中的getIP函数
    out<<type<<getUserName()<<localHostName;
    //将消息类型type和用户名和主机名按照先后顺序送到out

    switch (type) {
    case Message:
        if(ui->textEdit->toPlainText()==""){
            QMessageBox::warning(this,tr("警告"),tr("发送的内容不能为空！"),QMessageBox::Ok);
            return;
        }
        out<<address<<getMessage();//将IP地址和消息内容输入到out数据流
        ui->textBrowser->verticalScrollBar()->setValue(ui->textBrowser->verticalScrollBar()->maximum());
        //设置垂直滚动条的最大值，需要加头文件#include<QScrollBar>
        break;
    case NewParticipant:
        out<<address;//将IP地址输入到out数据流
        break;
    case ParticipantLeft:
        break;
    case FileName:
        break;
    case Refuse:
        break;
    default:
        break;
    }

}

void Widget::on_pushButton_send_clicked()
{
    
}
