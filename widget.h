#ifndef WIDGET_H
#define WIDGET_H
#include <QWidget>
#include<QUdpSocket>
namespace Ui {
class Widget;
}
enum MessageType //枚举变量标志信息的类型
{
    Message,//消息
    NewParticipant,//新用户加入
    ParticipantLeft,//用户退出
    FileName,//文件名
    Refuse
};//拒绝接受文件
class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();
protected:
    void newParticipant(QString userName,QString localHostName,QString ipAddress);//处理新用户
    void participantLeft(QString userName,QString localHostName,QString time);//用户离开
   
    void sendMessage(MessageType type,QString serverAddress="");//发送广播消息
    
    QString getIP();
    
    QString getUserName();
    
    QString getMessage();

private slots:
    void on_pushButton_send_clicked();
    
    void processPendingDatagrams();
    
private:
    Ui::Widget *ui;
    QUdpSocket *udpSocket;
    qint16 port;
};

#endif // WIDGET_H
