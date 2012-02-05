#ifndef PMSYSTEM_H
#define PMSYSTEM_H

#include <QtGui>

#include "logmanager.h"
#include "../Utilities/functions.h"
#include "../Utilities/otherwidgets.h"

class QScrollDownTextBrowser;
class QIRCLineEdit;
class PMStruct;

class PMSystem : public QWidget {
    Q_OBJECT

public:
    PMSystem(bool withTabs);
    ~PMSystem();

    void startPM(PMStruct *newPM);
    void checkTabbing();

private slots:
    void closeTab(int tabNum);
    void tabChanged(int tabNum);
    void togglePMs(bool toggled);
    void messageReceived(PMStruct *pm);

private:
    QExposedTabWidget *myPMs;
    bool tabbedPMs;
    QHash<int, PMStruct*> myPMWindows;
};

struct PMStruct : public QWidget
{
    Q_OBJECT
    PROPERTY(int, id);
public:
    PMStruct(int id, const QString &ownName, const QString &name, const QString &content = "", bool html = false, bool pmDisabled = false, int starterAuth = 0);
    ~PMStruct() {
        emit destroyed(id(), m_name);
    }

    void changeName(const QString &newname);
    void changeSelf(const QString &newname);
    void printLine(QString line, bool self = false);
    void disable();
    void reuse(int id);

    QString name() const {
        return m_name;
    }

    enum State {
        NoMessages,
        NewMessage
    };

    int state;

signals:
    void messageReceived(PMStruct *pm);
    void messageEntered(int id, const QString &mess);
    void challengeSent(int id);
    void destroyed(int id, QString name);
    void ignore(int id, bool);

public slots:
    void sendMessage();
    void ignore(bool);
    void challenge();
    void disablePM(bool, int starterAuth);

protected:
    void closeEvent(QCloseEvent *event);

private:
    QString m_name;
    QString m_ownName;
    bool escape_html;

    Log *log;

    void printHtml(const QString &htmlCode, bool timestamps = true);

    QScrollDownTextBrowser *m_mainwindow;
    QIRCLineEdit *m_textToSend;
    QPushButton *m_challenge, *m_send;
};

#endif // PMSYSTEM_H
