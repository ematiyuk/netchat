#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>

#include "client.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QMainWindow *parent = 0);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event);
    bool eventFilter(QObject *object, QEvent *event);

private:
    Ui::MainWindow *ui;
    Client* client;

    QAction *openAction;
    QAction *quitAction;
    QSystemTrayIcon *trayIcon;
    QMenu *trayIconMenu;
    bool someFlag;

    void setDefaults();
    void adjustGUIOnClientDisconnected();
    void tryToSendMessage();
    QString retrieveUUIDFromStr(QString str);
    QVariant loadOneSetting(const QString &key, const QVariant &defaultValue);
    void saveOneSetting(const QString &key, const QVariant &value);
    void signInIfNecessary();
    void tryToConnectToChatServer();
    void setIcon();
    void createActions();
    void createTrayIcon();
    void saveHistory();
    void loadHistory();
    QString prepareForSaving(QString text);

public slots:
    void onAddToLogArea(const QString &text, bool emptyLineIsNeeded = true);
    void onAddClientsToGUI(const QStringList &clientsList);
    void onAddClientToGUI(const QString &uuid, const QString &name);
    void onRemoveClientFromGUI(const QString &uuid, const QString &name);
    void onClientDisconnected();
    void onClearMessageArea();
    void onSetWindowTitleWithClientName();
    void onSetWindowTitleNoAuth();
    void onAdjustGUIOnDeregister();
    void onShowMessageInTray(const QString &title, const QString &msg,
                             QSystemTrayIcon::MessageIcon icon, int msecs, bool isMsg);

private slots:
    void on_pbConnect_clicked();
    void on_pbDisconnect_clicked();
    void on_pbSend_clicked();
    void on_cbToAll_toggled(bool checked);
    void on_cbAutoSignIn_toggled(bool checked);
    void signIn();
    void on_pbSignInOut_clicked();
    void iconActivated(QSystemTrayIcon::ActivationReason reason);
    void openMainWindow();
    void onMessageClicked();
};

#endif // MAINWINDOW_H
