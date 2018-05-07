#include "widget.h"
#include "ui_widget.h"
#include<QByteArray>
#include<QHostInfo>
#include<QDataStream>
#include<QMessageBox>
#include<QScrollBar>
#include<QDateTime>
#include<QTableWidgetItem>
#include<QNetworkInterface>
#include<QProcess>
#include<QStringList>
#include<QList>
#include<tcpserver.h>
#include<tcpclient.h>
#include<QFileDialog>
#include<QColorDialog>
#include<QFont>
#include<QKeyEvent>
#include<chat.h>
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);
    ui->textEdit->setFocusPolicy(Qt::StrongFocus);
    ui->textBrowser->setFocusPolicy(Qt::NoFocus);
    ui->textEdit->setFocus();
    ui->textEdit->installEventFilter(this);//设置完后自动调用其eventFilter函数

    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);//设置tablewidget不可以编辑

    //UDP部分
    udpSocket=new QUdpSocket(this);
    port=45454;
    udpSocket->bind(port,QUdpSocket::ShareAddress|QUdpSocket::ReuseAddressHint);
    //绑定端口号，采用ShareAddress模式（可以允许其他的服务器连接到相同的地址和端口，特别是用在
    //多客户端监听同一个服务器端口时特别有效），ReuseAddressHint为QUdpSocke提供提示，
    //即在地址和端口已经被其他套接字绑定的情况下，也应该试着重新绑定
    connect(udpSocket,SIGNAL(readyRead()),this,SLOT(processPendingDatagrams()));
            //在接口里有信息时，立马处理
    sendMessage(NewParticipant);//打开此软件，就说明是新用户加入，所以发射新用户加入广播
    //TcpServer是tcpserver.ui对应的类，上面直接用QUdpSocket是因为没有单独的udpserver.ui类

    //TCP部分
    server = new TcpServer(this);
        //sendFileName()函数一发送，则触发槽函数getFileName()
    connect(server, SIGNAL(sendFileName(QString)), this, SLOT(getFileName(QString)));

    connect(ui->textEdit,SIGNAL(currentCharFormatChanged(QTextCharFormat)),this,SLOT(curFmtChanged(const QTextCharFormat)));
            //使得光标在不同格式的文本上单击时可以使得编辑器自动切换为对应的格式

}

Widget::~Widget()
{
    delete ui;
}

//使用UDP广播发送消息，MessageType是指头文件中的枚举数据类型
//sendMessage即把本机的主机名，用户名+消息内容+IP地址再广播出去
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
    case FileName:{
        int row = ui->tableWidget->currentRow();//必须选中需要发送的给谁才可以发送
        QString clientAddress = ui->tableWidget->item(row,2)->text();//（row,2）为ip地址
        out << address << clientAddress << fileName;//发送本地ip，对方ip，所发送的文件名
        break;
    }
    case Refuse:
        out << serverAddress;
        break;
    }
    udpSocket->writeDatagram(data,data.length(),QHostAddress::Broadcast,port);
    //将data中的数据发送，QHostAddress::Broadcast是广播发送
}

//接收UDP信息
void Widget::processPendingDatagrams()
{
    while(udpSocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpSocket->pendingDatagramSize());//把datagram的大小设置为接受到的数据报的大小
        udpSocket->readDatagram(datagram.data(),datagram.size());
        QDataStream in(&datagram,QIODevice::ReadOnly);//输入，设置只读属性
        int messageType;
        in>>messageType;//读取1个32位长度的整型数据到messageType中
        QString userName,localHostName,ipAddress,message;
        QString time=QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        switch (messageType) {
        case Message:
            in>>userName>>localHostName>>ipAddress>>message;//in>>后面如果为QString，则表示读取一个直到‘\0’的字符串
            ui->textBrowser->setTextColor(Qt::blue);//设置文本颜色
            ui->textBrowser->setCurrentFont(QFont("Times New Roman",12));
            ui->textBrowser->append("["+localHostName+"]"+time);
            ui->textBrowser->append(message);//消息输出
            break;
        case NewParticipant:
            in>>userName>>localHostName>>ipAddress;
            newParticipant(userName,localHostName,ipAddress);//新用户加入
            break;
        case ParticipantLeft:
            in>>userName>>localHostName;
            participantLeft(userName,localHostName,time);
            break;
        case FileName:{
            in >> userName >> localHostName >> ipAddress;
            QString clientAddress, fileName;
            in >> clientAddress >> fileName;
            hasPendingFile(userName, ipAddress, clientAddress, fileName);//判断是否要接受该文件，而且是否是广播给它的
            break;
        }
        case Refuse:{
            in >> userName >> localHostName;
            QString serverAddress;
            in >> serverAddress;
            QString ipAddress = getIP();
            if(ipAddress == serverAddress)
            {
                server->refused();
            }
            break;
        }
        case xchat:{
            in>>userName>>localHostName>>ipAddress;
            showxchat(localHostName,ipAddress);//显示与主机名聊天中，不是用户名
            break;
        }
        }

    }
}

//处理新用户
void Widget::newParticipant(QString userName,QString localHostName,QString ipAddress)
{
    //看是否是新用户，找一找列表里面是否有重复
    bool isEmpty=ui->tableWidget->findItems(localHostName,Qt::MatchExactly).isEmpty();
    //此处的findItems表示找到与内容localHostName匹配的item
    if(isEmpty)
    {
        //新建3个小的item，分别为user,host,ip
        QTableWidgetItem *user=new QTableWidgetItem(userName);
        QTableWidgetItem *host=new QTableWidgetItem(localHostName);
        QTableWidgetItem *ip=new QTableWidgetItem(ipAddress);

        ui->tableWidget->insertRow(0);//把新来的用户放在最上面
        ui->tableWidget->setItem(0,0,user);
        ui->tableWidget->setItem(0,1,host);
        ui->tableWidget->setItem(0,2,ip);

        ui->textBrowser->setTextColor(Qt::gray);//这是设置上面字体的颜色
        ui->textBrowser->setCurrentFont(QFont("Times New Roman",10));
        ui->textBrowser->append(tr("%1在线！").arg(userName));
        ui->label_num->setText(tr("在线人数：%1").arg(ui->tableWidget->rowCount()));
        sendMessage(NewParticipant);//该句的功能是让新来的用户也能收到其他在线用户的信息，可以
        //更新自己的好友列表      
    }
}
//处理用户离开
void Widget::participantLeft(QString userName, QString localHostName, QString time)
{
    //找到第一个对应的主机名
    int rowNum=ui->tableWidget->findItems(localHostName,Qt::MatchExactly).first()->row();
    ui->tableWidget->removeRow(rowNum);//移除，此句执行完后，rowcount()内容自动减1
    ui->textBrowser->setTextColor(Qt::gray);
    ui->textBrowser->setCurrentFont(QFont("Times New Roman",10));
    ui->textBrowser->append(tr("%1于%2离开！").arg(userName).arg(time));
    ui->label_num->setText(tr("在线人数:%1").arg(ui->tableWidget->rowCount()));
}
//获取ip地址，获取本机ip地址（其协议为ipv4的ip地址）
QString Widget::getIP()
{
    QList<QHostAddress> list=QNetworkInterface::allAddresses();//此处包含所有的ipv4和ipv6的地址
    foreach(QHostAddress address,list)
    {
        if(address.protocol()==QAbstractSocket::IPv4Protocol)
            return address.toString();
    }
    return 0;
}


//获取用户名????
QString Widget::getUserName()
{
    QStringList envVariables;
    //将这5个环境变量存入到envVariables
    envVariables << "USERNAME.*" << "USER.*" << "USERDOMAIN.*"
                     << "HOSTNAME.*" << "DOMAINNAME.*";
    //系统中关于环境变量的信息存在environment中
    QStringList environment=QProcess::systemEnvironment();
    foreach(QString string,envVariables)
    {
        int index=environment.indexOf(QRegExp(string));
        if(index!=-1)
        {
            QStringList stringList=environment.at(index).split('=');
            if(stringList.size()==2)
            {
                return stringList.at(1);//at(0)为文字"USERNAME."，at(1)为用户名
                break;
            }
        }
    }
    return "unknown";
}
//获得,要发送的消息，放在socket中发出去
QString Widget::getMessage()
{
    QString msg=ui->textEdit->toHtml();//转成html语言发送
    ui->textEdit->clear();//清空输入框
    ui->textEdit->setFocus();//重新设置光标输入焦点
    return msg;

}

//发送信息
void Widget::on_pushButton_send_clicked()
{
    sendMessage(Message);
}
// 获取要发送的文件名
void Widget::getFileName(QString name)
{
    fileName = name;
    sendMessage(FileName);
}
//传输文件按钮
void Widget::on_toolButton_sendfile_clicked()
{
    if(ui->tableWidget->selectedItems().isEmpty())//传送文件前需选择用户
        {
            QMessageBox::warning(0, tr("选择用户"),
                           tr("请先从用户列表选择要传送的用户！"), QMessageBox::Ok);
            return;
        }
        server->show();
       server->initServer();
}
// 是否接收文件，客户端的显示
void Widget::hasPendingFile(QString userName, QString serverAddress,
                            QString clientAddress, QString fileName)
{
    QString ipAddress = getIP();
    if(ipAddress == clientAddress)//传过来的客户端IP地址和本地相同，说明这个就是它要传的客户端
    {
        int btn = QMessageBox::information(this,tr("接受文件"),
                                           tr("来自%1(%2)的文件：%3,是否接收？")
                                           .arg(userName).arg(serverAddress).arg(fileName),
                                           QMessageBox::Yes,QMessageBox::No);//弹出一个窗口
        if (btn == QMessageBox::Yes) {//如果接受，则首先创建一个TCP客户端，然后双方进行一个TCP连接进行文件的传输
            QString name = QFileDialog::getSaveFileName(0,tr("保存文件"),fileName);//name为另存为的文件名
            if(!name.isEmpty())
            {
                TcpClient *client = new TcpClient(this);
                client->setFileName(name);    //客户端设置文件名
                client->setHostAddress(QHostAddress(serverAddress));    //客户端设置服务器地址
                client->show();
            }
        } else {//如果拒绝接收，则发送拒绝消息的广播
            sendMessage(Refuse, serverAddress);//serverAddress传过来其实是对方的IP地址
        }
    }
}
bool Widget::eventFilter(QObject *target, QEvent *event)
{
    if(target==ui->textEdit)
    {
        if(event->type()==QEvent::KeyPress)//回车键
        {
            QKeyEvent *k=static_cast<QKeyEvent *>(event);
            if(k->key()==Qt::Key_Return)
            {
                on_pushButton_send_clicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(target,event);
}


//改变字体
void Widget::on_fontComboBox_currentFontChanged(const QFont &f)
{
    ui->textEdit->setCurrentFont(f);
    ui->textEdit->setFocus();
}
//更改字号
void Widget::on_comboBox_fontsize_currentIndexChanged(const QString &arg1)
{
    ui->textEdit->setFontPointSize(arg1.toDouble());
    ui->textEdit->setFocus();
}
//加粗
void Widget::on_toolButton_bold_clicked(bool checked)
{
    if(checked)
        ui->textEdit->setFontWeight(QFont::Bold);
    else
        ui->textEdit->setFontWeight(QFont::Normal);
    ui->textEdit->setFocus();
}
//设置斜体
void Widget::on_toolButton_italic_clicked(bool checked)
{
    ui->textEdit->setFontItalic(checked);
    ui->textEdit->setFocus();
}
//设置下划线
void Widget::on_toolButton_underline_clicked(bool checked)
{
    ui->textEdit->setFontUnderline(checked);
    ui->textEdit->setFocus();
}
//设置文本颜色
void Widget::on_toolButton_color_clicked()
{
    color=QColorDialog::getColor(color,this);
    if(color.isValid())
    {
        ui->textEdit->setTextColor(color);
        ui->textEdit->setFocus();
    }
}

void Widget::curFmtChanged(const QTextCharFormat &fmt)
{
    ui->fontComboBox->setCurrentFont(fmt.font());//设置字体
    if(fmt.fontPointSize()<8)//在这里最小的字体为8，如果字体大小出错，则使用12大小
    {
        ui->comboBox_fontsize->setCurrentIndex(4);
    }else{
        ui->comboBox_fontsize->setCurrentIndex(ui->comboBox_fontsize->findText(QString::number(fmt.fontPointSize())));
    }
    ui->toolButton_bold->setChecked(fmt.font().bold());//设置粗体
    ui->toolButton_italic->setChecked(fmt.font().italic());//设置斜体
    ui->toolButton_underline->setChecked(fmt.font().underline());//设置下划线
    color=fmt.foreground().color();

}
//单击保存按钮
void Widget::on_toolButton_save_clicked()
{
    if(ui->textBrowser->document()->isEmpty())
    {
        QMessageBox::warning(this,tr("警告"),tr("聊天记录为空！无法保存！"),QMessageBox::Ok);
    }
    else
    {
        QString fname=QFileDialog::getSaveFileName(this,tr("保存聊天记录"),tr("聊天记录"),tr("文本(*.txt);;所有文件(*.*)"));
        if(!fname.isEmpty())
            saveFile(fname);
    }
}

//保存文件
bool Widget::saveFile(const QString& filename)
{
    QFile file(filename);
    if(!file.open(QFile::WriteOnly | QFile::Text))
    {
        QMessageBox::warning(this,tr("保存文件"),tr("无法保存文件%1:\n%2").arg(filename).arg(file.errorString()));
        return false;
    }
    QTextStream out(&file);
    out<<ui->textBrowser->toPlainText();
    return true;
}
//清空
void Widget::on_toolButton_clear_clicked()
{
    ui->textBrowser->clear();
}
//关闭
void Widget::on_pushButton_close_clicked()
{
    close();
}

void Widget::closeEvent(QCloseEvent *e)
{
    sendMessage(ParticipantLeft);
    QWidget::closeEvent(e);
}

void Widget::on_tableWidget_doubleClicked(const QModelIndex &index)//双击出现私聊窗口
{

   if(ui->tableWidget->item(index.row(),0)->text()==getUserName() &&
            ui->tableWidget->item(index.row(),2)->text()==getIP())
    {
        QMessageBox::warning(this,tr("警告"),tr("你不可以和自己聊天！！！"),QMessageBox::Ok);
    }
    else{
//        if(!privatechat)
//        {
           privatechat=new chat(ui->tableWidget->item(index.row(),1)->text(),//接收主机名
                                 ui->tableWidget->item(index.row(),2)->text());//接收用户IP
            QByteArray data;
            MessageType xchat;
            QDataStream out(&data,QIODevice::WriteOnly);
            QString localHostName = QHostInfo::localHostName();
            QString address = getIP();
            out << xchat << getUserName() << localHostName << address;//输入type=xchat
            udpSocket->writeDatagram(data,data.length(),QHostAddress(ui->tableWidget->item(index.row(),2)->text()), port);//特定的IP地址，而不是之前的广播


//            privatechat->show();
//            privatechat->is_opened = true;

       // }
    }
    /*privatechat=new chat(ui->tableWidget->item(index.row(),1)->text(),//接收主机名
                         ui->tableWidget->item(index.row(),2)->text());//接收用户IP
    privatechat->show();*/
    /*privatechat=new chat(this);
    privatechat->show();*/
}

void Widget::showxchat(QString name, QString ip)
{
//    privatechat=new chat(ui->tableWidget->item(index.row(),1)->text(),//接收主机名
//                         ui->tableWidget->item(index.row(),2)->text());//接收用户IP
    privatechat->show();
    privatechat->is_opened = true;
//    if(!privatechat1)
//    {
//        privatechat1=new chat(name,ip);
//    }
}
