#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QDebug>
#include <QTcpSocket>
#include <QtGui>
#include <QRegExp>

#include "server.h"
#include "utils.h"

class ChatServer;

class Client : public QObject
{
    friend class ChatServer;
    Q_OBJECT

public:
    explicit Client(qintptr socketDesc, ChatServer *chatServerPtr, QObject *parent = 0);
    ~Client();

    void setUUID(QString uuid) {this->clientUUID = uuid;}
    QString getUUID() const {return this->clientUUID;}
    void setName(QString name) {this->clientName = name;}
    QString getName() const {return this->clientName;}
    void setRegistered(bool isRegFlag = false) {this->isReg = isRegFlag;}
    bool isRegistered() const {return this->isReg;}
    void sendCommand(quint8 comm) const;
    void sendRegisteredClients() const;

private:
    QString clientUUID;
    QString clientName;
    QTcpSocket *socket;
    qintptr socketDescriptor;
    quint16 blockSize;

    ChatServer *chatServer;
    bool isReg;
    Utils *utils;

signals:
    void addClientToGui(QString clientUUID, QString clientName);
    void addToLogArea(const QString &text, bool emptyLineIsNeeded = true);
    void removeClientFromGui(QString clientUUID, QString clientName);
    void removeClient(Client *client);
    void messageToGui(QString message, QString from, const QStringList &clients);
    void stopChatServer();

private slots:
    void onConnect();
    void onDisconnect();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError socketError) const;
};

#endif // CLIENT_H
