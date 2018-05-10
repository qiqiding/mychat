#ifndef CHAT_H
#define CHAT_H
#include<QUdpSocket>
#include <QDialog>
#include<QTcpServer>
#include<tcpserver.h>
#include<tcpclient.h>
#include<QTextCharFormat>
namespace Ui {
class chat;
}

enum MessageType //枚举变量标志信息的类型
{
    Message,//消息
    NewParticipant,//新用户加入
    ParticipantLeft,//用户退出
    FileName,//文件名
    Refuse,//拒绝接受文件
    xchat//私聊窗口
};
class chat : public QDialog
{
    Q_OBJECT

public:
    explicit chat(QWidget *parent = 0);
    ~chat();
    chat(QString pasvusername, QString pasvuserip);//重载构造函数，就是为了把参数传递过来
    QString xpasvusername;
    QString xpasvuserip;
    QUdpSocket *udpsocket;
    qint32 xport;
    void sendMessage(MessageType type,QString serverAddress="");

  //  bool is_opened;
protected:
    bool eventFilter(QObject *target, QEvent *event);//事件过滤器（用来textedit的enter）

    void hasPendingFile(QString userName,QString serverAddress,  //接收文件
                                  QString clientAddress,QString fileName);
    void participantLeft(QString userName,QString localHostName,QString time);//处理用户离开


private:
    Ui::chat *ui;
    TcpServer *server;
    QColor color;//颜色
    bool saveFile(const QString& fileName);//保存聊天记录
    QString getMessage();
    QString getIP();
    QString getUserName();
    QString message;
    QString fileName;
public slots:
    void sentFileName(QString filename);
    void processPendingDatagrams();//处理UDP消息

private slots:
    void on_send_clicked();
    void on_fontComboBox_currentFontChanged(const QFont &f);
    void on_fontsizecomboBox_currentIndexChanged(const QString &arg1);
    void on_textbold_clicked(bool checked);
    void on_textitalic_clicked(bool checked);
    void on_save_clicked();
    void on_clear_clicked();
    void on_textUnderline_clicked(bool checked);
    void on_textcolor_clicked();
    void on_close_clicked();
    void on_sendfile_clicked();
    void currentFormatChanged(const QTextCharFormat &format);
    //void on_textUnderline_clicked();
};

#endif // CHAT_H
