#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QDebug>
#include <QtGui>
#include <QtCore>

#include "server.h"
#include "utils.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QMainWindow *parent = 0);
    ~MainWindow();

    void setVisible(bool visible);

protected:
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *object, QEvent *event);

private:
    Ui::MainWindow *ui;
    ChatServer* chatServer;

    QAction *startAction;
    QAction *stopAction;
    QAction *minimizeAction;
    QAction *restoreAction;
    QAction *quitAction;
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    Utils *utils;

    void setDefaults();
    void addToLogArea(const QString &text, bool emptyLineIsNeeded = true);
    void adjustGUIOnServerStopped();
    void tryToSendMessage();
    QString retrieveUUIDFromStr(QString str);
    QString retrieveNameFromStr(QString str);
    QVariant loadOneSetting(const QString &key, const QVariant &defaultValue);
    void saveOneSetting(const QString &key, const QVariant &value);
    void startChatServerIfNecessary();
    void tryToConnectToChatServer();
    void setIcon();
    void createActions();
    void createTrayIcon();

signals:
    void messageFromGui(QString message, const QStringList &users);

private slots:
    void on_pbSend_clicked();
    void on_cbToAll_clicked();
    void on_pbStart_clicked();
    void on_pbStop_clicked();
    void tryToStartChatServer();
    void on_cbAutostart_toggled(bool checked);
    void iconActivated(QSystemTrayIcon::ActivationReason reason);

public slots:
    void onAddClientToGui(QString uuid, QString name);
    void onRemoveClientFromGui(const QString &uuid, const QString &name);
    void onMessageToGui(QString message, QString senderClientName, const QStringList &receiversList);
    void onAddToLogArea(const QString &text, bool emptyLineIsNeeded);
    void onStopChatServer();
    void onClearMessageArea();
};

#endif // MAINWINDOW_H
