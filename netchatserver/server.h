#ifndef SERVER_H
#define SERVER_H

#include <QMainWindow>
#include <QTcpServer>
#include <QDebug>

#include "client.h"

class QTcpSocket;
class QHostInfo;
class QWidget;

class Client;

class ChatServer : public QTcpServer {
    Q_OBJECT

public:
    explicit ChatServer(QMainWindow *widget = 0, QObject *parent = 0);

private:
    QString srvHost;
    quint16 srvPort;
    QList<Client *> clientsList;
    QList<QString> reservedNamesList;
    QWidget *mainWindow;

    void fillReservedNamesList();
    QString retrieveUUIDFromStr(QString str);
    quint16 getRegisteredClientsQuantity();

protected:
    void incomingConnection(qintptr handle);

public:
    QString getServeclient() {return this->srvHost;}
    quint16 getServerPort() {return this->srvPort;}
    void setServerPort(quint16 port) {this->srvPort = port;}
    void setServerHost(QString host) {this->srvHost = host;}
    QList<Client *> getClientsList() {return this->clientsList;}

    bool isCommandExpected(QString text);
    void processCommand(QString text);

    bool startChatServer(QHostAddress ipAddress, qint16 port);
    void sendCommand(quint8 comm, QString uuid);
    void sendToAllHasJoined(QString uuid, QString name);
    void sendToAllHasLeft(QString uuid, QString name);
    void sendToAllMessage(QString message, QString fromClientUUID, QString fromClientName);
    void sendToAllServerMessage(QString message);
    void sendServerMessageToClients(QString message, const QStringList &clients);
    void sendMessageToClients(QString message, const QStringList &agentsReceiversList,
                              QString fromAgentUUID, QString fromAgentName);
    void sendAdvertisementToInitiator(QPixmap pixmap, QString initiatorUUID,
                                      QString fromAgentUUID, QString fromAgentName);
    QStringList getRegisteredClients() const;
    bool isNameUsed(QString name) const;
    bool isNameIllegal(QString name) const;
    bool clientExists(QString uuid) const;

    void sendCommandToAll(quint8 command);

    void deregisterAll();

signals:
    void addToLogArea(const QString &text, bool emptyLineIsNeeded = true);
    void removeClientFromGui(const QString &uuid, const QString &name);
    void clearMessageArea();

public slots:
    void sendMessageFromServer(QString message, const QStringList &agents);
    void onRemoveClient(Client *client);
};

#endif // SERVER_H
