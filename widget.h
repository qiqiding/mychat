#ifndef WIDGET_H
#define WIDGET_H
#include <QWidget>
#include<QUdpSocket>
#include<QColor>
#include<QTextCharFormat>
#include<chat.h>
namespace Ui {
class Widget;
}

class TcpServer;//定义那个自己定义的类
enum MessageType //枚举变量标志信息的类型
{
    Message,//消息
    NewParticipant,//新用户加入
    ParticipantLeft,//用户退出
    FileName,//文件名
    Refuse,//拒绝接受文件
    xchat//私聊窗口
};
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
    
    QString getIP();//获取ip地址
    
    QString getUserName();//获取用户名
    
    QString getMessage();//获取聊天信息

    void hasPendingFile(QString userName, QString serverAddress,
                            QString clientAddress, QString fileName);// 用于在收到文件名UDP消息时判断是否接收该文件

    bool saveFile(const QString& filename);//保存聊天记录

    void showxchat(QString name,QString ip);//显示私聊窗口

    void closeEvent(QCloseEvent *);

    //void changeEvent(QEvent *e);

    bool eventFilter(QObject *target, QEvent *event);//事件过滤器（用来textedit的enter）

private slots:
    void on_pushButton_send_clicked();//发送udp消息
    
    void processPendingDatagrams();//接收udp消息
    
    void on_toolButton_sendfile_clicked();//传送文件

    void getFileName(QString);//来获取服务器类sendFileName()信号发送过来的文件名


    void on_fontComboBox_currentFontChanged(const QFont &f);

    void on_comboBox_fontsize_currentIndexChanged(const QString &arg1);



    void on_toolButton_bold_clicked(bool checked);

    void on_toolButton_italic_clicked(bool checked);

    void on_toolButton_underline_clicked(bool checked);

    void on_toolButton_color_clicked();

    void curFmtChanged(const QTextCharFormat &fmt);
    //使得光标在不同格式的文本上单击时可以使得编辑器自动切换为原先对应的格式
    void on_toolButton_save_clicked();

    void on_toolButton_clear_clicked();

    void on_pushButton_close_clicked();

    void on_tableWidget_doubleClicked(const QModelIndex &index);

private:
    Ui::Widget *ui;
    QUdpSocket *udpSocket;
    qint16 port;

    QString fileName;
    TcpServer *server;

    QColor color;
    chat *privatechat;
    chat *privatechat1;
};

#endif // WIDGET_H
