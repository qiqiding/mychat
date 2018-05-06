#ifndef CHAT_H
#define CHAT_H

#include <QDialog>

namespace Ui {
class chat;
}

class chat : public QDialog
{
    Q_OBJECT

public:
    explicit chat(QWidget *parent = 0);
    ~chat();
    chat(QString pasvusername, QString pasvuserip);
    bool is_opened;

private:
    Ui::chat *ui;
};

#endif // CHAT_H
