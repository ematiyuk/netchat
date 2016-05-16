#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QMainWindow>
#include <QTcpSocket>
#include <QSystemTrayIcon>
#include <QMediaPlayer>

class Utils;

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QMainWindow *widget = 0, QObject *parent = 0);

private:
    QString uuid;
    QString clientName;
    QTcpSocket *socket;
    quint16 blockSize;

    QMainWindow* mainWindow;
    QMediaPlayer *msgSound;
    Utils *utils;

public:
    void setName(const QString &name) {this->clientName = name;}
    void setUUID(const QString &uuid) {this->uuid = uuid;}
    void setSocket(QTcpSocket *socket) {this->socket = socket;}
    QString getName() {return clientName;}
    QString getUUID() {return uuid;}
    QTcpSocket* getSocket() {return socket;}

    bool connectToChatServer(QHostAddress ipAddress, quint16 port);
    void disconnectFromChatServer();
    void sendMessageToAll(QString message);
    void sendMessageToSelected(QString message, QString selectedClients);
    bool isConnected();
    bool isCommandExpected(QString text);
    void processCommand(QString text);
    void tryToRegister(QString name);
    void sendCommand(quint8 command);

private:
    QString generateUUID();
    QString retrieveNameFromStr(QString str);
    void writeToSocket(QByteArray block);

signals:
    void addToLogArea(const QString &, bool = true);
    void addClientsToGUI(const QStringList &);
    void addClientToGUI(const QString &, const QString &);
    void removeClientFromGUI(const QString &, const QString &);
    void clientDisconnected();
    void clearMessageArea();
    void setWindowTitleWithClientName();
    void adjustGUIOnDeregister();
    void showMessageInTray(const QString &title, const QString &msg,
                           QSystemTrayIcon::MessageIcon icon, int msecs, bool isMsg = true);

private slots:
    void onSocketDisplayError(QAbstractSocket::SocketError);
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketReadyRead();
    void sendClientConnected();
};

#endif // CLIENT_H
