#include "chat.h"
#include "ui_chat.h"
#include<QKeyEvent>
#include<QMessageBox>
#include<QDebug>
#include<QColorDialog>
#include<QHostAddress>
#include<QFileDialog>
#include<QHostInfo>
#include<QProcess>
#include<QNetworkInterface>
#include<QScrollBar>
chat::chat(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::chat)
{
    ui->setupUi(this);
}

chat::~chat()
{
    delete ui;
}
chat::chat(QString pasvusername, QString pasvuserip) : ui(new Ui::chat)
{
    ui->setupUi(this);
    ui->textEdit->setFocusPolicy(Qt::StrongFocus);
    ui->textBrowser->setFocusPolicy(Qt::NoFocus);

    ui->textEdit->setFocus();
    ui->textEdit->installEventFilter(this);
    is_opened = false;

    xpasvusername=pasvusername;//接收传过来的用户名
    xpasvuserip=pasvuserip;//接收传过来的IP地址
    ui->label->setText(tr("与%1聊天中 对方IP：%2").arg(xpasvusername).arg(xpasvuserip));

    //UDP部分
    udpsocket=new QUdpSocket(this);
    xport=45456;
    udpsocket->bind(QHostAddress(xpasvuserip),xport);//用对方的IP地址建立一个udpsocket,不是之前的广播形式
    connect(udpsocket,SIGNAL(readyRead()),this,SLOT(processPendingDatagrams()));

    //TCP部分
    server=new TcpServer(this);
    connect(server,SIGNAL(sendFileName(QString)),this,SLOT(sentFileName(QString)));
    //connect(ui->textEdit,SIGNAL())

    connect(ui->textEdit,SIGNAL(currentCharFormatChanged(QTextCharFormat)),this,SLOT(currentFormatChanged(const QTextCharFormat)));

}
bool chat::eventFilter(QObject *target, QEvent *event)//处理textedit里面的enter把文字发送出去
{
        if(target==ui->textEdit)
        {
            if(event->type()==QEvent::KeyPress)//回车键
            {
                QKeyEvent *k=static_cast<QKeyEvent *>(event);
                if(k->key()==Qt::Key_Return)
                {
                    on_send_clicked();
                    return true;
                }
            }
        }
        return QWidget::eventFilter(target,event);
}
//处理用户离开
void chat::participantLeft(QString userName, QString localHostName, QString time)
{
    ui->textBrowser->setTextColor(Qt::gray);
    ui->textBrowser->setCurrentFont(QFont("Times New Roman",10));
    ui->textBrowser->append(tr("%1与%2离开！").arg(userName).arg(time));
}
//发送文件名
void chat::sentFileName(QString filename)
{
    this->fileName=filename;
    sendMessage(FileName);
}

void chat::processPendingDatagrams()//处理UDP消息
{
    while(udpsocket->hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(udpsocket->pendingDatagramSize());
        udpsocket->readDatagram(datagram.data(),datagram.size());
        QDataStream in(&datagram,QIODevice::ReadOnly);
        int messageType;
        in >> messageType;
        QString userName,localHostName,ipAddress,messagestr;
        QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        switch (messageType) {
        case Message:
        {
            in >>userName >>localHostName >>ipAddress >>messagestr;
            ui->textBrowser->setTextColor(Qt::blue);
            ui->textBrowser->setCurrentFont(QFont("Times New Roman",12));
            ui->textBrowser->append("[ " +localHostName+" ] "+ time);//与主机名聊天中
            ui->textBrowser->append(messagestr);

            //this->show();
            //is_opened=true;
             break;
        }
        case Refuse:
        {
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
        case FileName:
        {
              in >>userName >>localHostName >> ipAddress;
              QString clientAddress,fileName;
              in >> clientAddress >> fileName;
              hasPendingFile(userName,ipAddress,clientAddress,fileName);
              break;
         }

         case ParticipantLeft:
        {
            in>>userName>>localHostName;
            participantLeft(userName,localHostName,time);
            QMessageBox::information(this,tr("本次对话关闭"),tr("对方结束了对话"),QMessageBox::Ok);
            ui->textBrowser->clear();
            ui->~chat();
            close();
            break;
        }

        default:
            break;
        }
    }
}

//获取IP地址
QString chat::getIP()
{
//    QList<QHostAddress> list = QNetworkInterface::allAddresses();
//        foreach (QHostAddress address, list)
//        {
//           if(address.protocol() == QAbstractSocket::IPv4Protocol) //我们使用IPv4地址
//                return address.toString();
//        }
//           return 0;
    return xpasvuserip;
}

//获取用户名，得到的是主机名
QString chat::getUserName()
{
    QStringList envVariables;
        envVariables << "USERNAME.*" << "USER.*" << "USERDOMAIN.*"
                     << "HOSTNAME.*" << "DOMAINNAME.*";
        QStringList environment = QProcess::systemEnvironment();
        foreach (QString string, envVariables)
        {
            int index = environment.indexOf(QRegExp(string));
            if (index != -1)
            {

                QStringList stringList = environment.at(index).split('=');
                if (stringList.size() == 2)
                {
                    return stringList.at(1);
                    break;
                }
            }
        }
        return false;
}
//接受文件
void chat::hasPendingFile(QString userName, QString serverAddress, QString clientAddress, QString fileName)
{
    QString ipAddress = getIP();
       if(ipAddress == clientAddress)
       {
           int btn = QMessageBox::information(this,tr("接受文件"),
                                    tr("来自%1(%2)的文件：%3,是否接收？")
                                    .arg(userName).arg(serverAddress).arg(fileName),
                                    QMessageBox::Yes,QMessageBox::No);
           if(btn == QMessageBox::Yes)
           {
               QString name = QFileDialog::getSaveFileName(0,tr("保存文件"),fileName);
               if(!name.isEmpty())
               {
                   TcpClient *client = new TcpClient(this);
                   client->setFileName(name);
                   client->setHostAddress(QHostAddress(serverAddress));
                   client->show();

               }

           }
           else{
               sendMessage(Refuse,serverAddress);
           }
       }

}
//获得要发送的信息
QString chat::getMessage()
{
    QString msg=ui->textEdit->toHtml();
    qDebug()<<msg;
    ui->textEdit->clear();
    ui->textEdit->setFocus();
    return msg;
}
//通过私聊套接字发送到对方的私聊专用端口上
void chat::sendMessage(MessageType type, QString serverAddress)
{
        QByteArray data;
        QDataStream out(&data,QIODevice::WriteOnly);
        QString localHostName = QHostInfo::localHostName();
        QString address = getIP();
        out << type << getUserName() << localHostName;

        switch(type)
        {
        case ParticipantLeft:
            {
                break;
            }
        case Message :
            {
                if(ui->textEdit->toPlainText() == "")
                {
                    QMessageBox::warning(0,tr("警告"),tr("发送内容不能为空"),QMessageBox::Ok);
                    return;
                }
                message = getMessage();
                out << address << message;
                ui->textBrowser->verticalScrollBar()->setValue(ui->textBrowser->verticalScrollBar()->maximum());
                break;
            }
        case FileName:
                {
                    QString clientAddress = xpasvuserip;
                    out << address << clientAddress << fileName;
                    break;
                }
        case Refuse:
                {
                    out << serverAddress;
                    break;
                }
        }
    udpsocket->writeDatagram(data,data.length(),QHostAddress(xpasvuserip),xport);
}
void chat::currentFormatChanged(const QTextCharFormat &format)
{//当编辑器的字体格式改变时，我们让部件状态也随之改变
    ui->fontComboBox->setCurrentFont(format.font());

    if(format.fontPointSize()<9)  //如果字体大小出错，因为我们最小的字体为9
    {
        ui->fontsizecomboBox->setCurrentIndex(3); //即显示12
    }
    else
    {
        ui->fontsizecomboBox->setCurrentIndex(ui->fontsizecomboBox->findText(QString::number(format.fontPointSize())));

    }

    ui->textbold->setChecked(format.font().bold());
    ui->textitalic->setChecked(format.font().italic());
    ui->textUnderline->setChecked(format.font().underline());
    color = format.foreground().color();
}
//发送消息
void chat::on_send_clicked()
{
       sendMessage(Message);
       QString localHostName = QHostInfo::localHostName();
       QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
       ui->textBrowser->setTextColor(Qt::blue);
       ui->textBrowser->setCurrentFont(QFont("Times New Roman",12));
       ui->textBrowser->append("[ " +localHostName+" ] "+ time);
       ui->textBrowser->append(message);

}
//字体设置
void chat::on_fontComboBox_currentFontChanged(const QFont &f)
{
    ui->textEdit->setCurrentFont(f);
    ui->textEdit->setFocus();
}
//字号设置
void chat::on_fontsizecomboBox_currentIndexChanged(const QString &arg1)
{
    ui->textEdit->setFontPointSize(arg1.toDouble());
    ui->textEdit->setFocus();
}
//设置粗体
void chat::on_textbold_clicked(bool checked)
{
    if(checked)
    {
        ui->textEdit->setFontWeight(QFont::Bold);
    }else{
        ui->textEdit->setFontWeight(QFont::Normal);
    }
    ui->textEdit->setFocus();
}
//设置斜体
void chat::on_textitalic_clicked(bool checked)
{
    ui->textEdit->setFontItalic(checked);
    ui->textEdit->setFocus();
}
//保存聊天记录
void chat::on_save_clicked()
{
    if(ui->textBrowser->document()->isEmpty())
            QMessageBox::warning(0,tr("警告"),tr("聊天记录为空，无法保存！"),QMessageBox::Ok);
        else
        {
           //获得文件名
           QString fileName = QFileDialog::getSaveFileName(this,tr("保存聊天记录"),tr("聊天记录"),tr("文本(*.txt);;All File(*.*)"));
           if(!fileName.isEmpty())
               saveFile(fileName);
        }

}
bool chat::saveFile(const QString &fileName)
{
    QFile file(fileName);
        if(!file.open(QFile::WriteOnly | QFile::Text))

        {
            QMessageBox::warning(this,tr("保存文件"),
            tr("无法保存文件 %1:\n %2").arg(fileName)
            .arg(file.errorString()));
            return false;
        }
        QTextStream out(&file);
        out << ui->textBrowser->toPlainText();

        return true;

}

//清空聊天记录
void chat::on_clear_clicked()
{
    ui->textEdit->clear();
}
//下划线
void chat::on_textUnderline_clicked(bool checked)
{
    ui->textEdit->setFontUnderline(checked);
    ui->textEdit->setFocus();
}
//更改字体颜色
void chat::on_textcolor_clicked()
{
    color = QColorDialog::getColor(color,this);
        if(color.isValid())
        {
            ui->textEdit->setTextColor(color);
            ui->textEdit->setFocus();
        }

}
//按钮关闭
void chat::on_close_clicked()
{
    sendMessage(ParticipantLeft);
    ui->textBrowser->clear();
    close();
    ui->~chat();

}
//发送文件
void chat::on_sendfile_clicked()
{
    server->show();
    server->initServer();

}
