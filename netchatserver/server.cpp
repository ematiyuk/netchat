#include <QtNetwork>
#include <QtGui>
#include <QMessageBox>

#include "server.h"
#include "mainwindow.h"
#include "constants.h"

ChatServer::ChatServer(QMainWindow *widget, QObject *parent) : QTcpServer(parent)
{
    mainWindow = widget;
    fillReservedNamesList();

    QObject::connect(this, SIGNAL(addToLogArea(QString,bool)), mainWindow, SLOT(onAddToLogArea(QString,bool)));
    QObject::connect(this, SIGNAL(removeClientFromGui(QString,QString)), mainWindow, SLOT(onRemoveClientFromGui(QString,QString)));
    QObject::connect(this, SIGNAL(clearMessageArea()), mainWindow, SLOT(onClearMessageArea()));
}

bool ChatServer::startChatServer(QHostAddress ipAddress, qint16 port)
{
    if (!listen(ipAddress, port))
    {
        return false;
    }
    return true;
}

void ChatServer::sendCommand(quint8 comm, QString uuid)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0 << comm;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    quint16 clientsQuantity = getClientsList().length();
    for (int i = 0; i < clientsQuantity; ++i)
        if (getClientsList().at(i)->getUUID() == uuid)
        {
            getClientsList().at(i)->socket->write(block);
            break;
        }
}

void ChatServer::sendToAllHasJoined(QString uuid, QString name)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    // reserve space for block size
    out << (quint16)0 << Constants::comClientJoined << uuid << name;
    // write block size on reserved space
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    // send to all authorized except who has entered
    for (int i = 0; i < getClientsList().length(); ++i)
        if (getClientsList().at(i)->getUUID() != uuid && getClientsList().at(i)->isRegistered())
            getClientsList().at(i)->socket->write(block);
}

void ChatServer::sendToAllHasLeft(QString uuid, QString name)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0 << Constants::comClientLeft << uuid << name;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    for (int i = 0; i < getClientsList().length(); ++i)
        if (getClientsList().at(i)->getUUID() != uuid && getClientsList().at(i)->isRegistered())
            getClientsList().at(i)->socket->write(block);
}

void ChatServer::sendToAllMessage(QString message, QString fromClientUUID, QString fromClientName)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0 << Constants::comMessageToAll << fromClientUUID << fromClientName << message;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    for (int i = 0; i < getClientsList().length(); ++i)
        if (getClientsList().at(i)->isRegistered())
            getClientsList().at(i)->socket->write(block);
}

QString ChatServer::retrieveUUIDFromStr(QString str)
{
    return str.right(str.length() - str.indexOf('{'));
}

void ChatServer::sendMessageToClients(QString message, const QStringList &clientsReceiversList,
                                      QString fromClientUUID, QString fromClientName)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0 << Constants::comMessageToClients << clientsReceiversList;
    out << fromClientUUID << fromClientName << message;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));

    QStringList receiversUUIDsList;
    foreach (QString item, clientsReceiversList) {
        receiversUUIDsList.append(this->retrieveUUIDFromStr(item));
    }

    for (int j = 0; j < getClientsList().length(); ++j)
        if (receiversUUIDsList.contains(getClientsList().at(j)->getUUID()))
            getClientsList().at(j)->socket->write(block);         // to receivers
        else if (getClientsList().at(j)->getUUID() == fromClientUUID)
            getClientsList().at(j)->socket->write(block);         // to sender
}

void ChatServer::sendToAllServerMessage(QString message)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0 << Constants::comPublicServerMessage << message;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    for (int i = 0; i < getClientsList().length(); ++i)
        if (getClientsList().at(i)->isRegistered())
            getClientsList().at(i)->socket->write(block);
}

void ChatServer::sendServerMessageToClients(QString message, const QStringList &clients)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0 << Constants::comPrivateServerMessage << message;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));

    QStringList clientsUUIDsList;
    foreach (QString item, clients) {
        clientsUUIDsList.append(this->retrieveUUIDFromStr(item));
    }

    for (int j = 0; j < this->getClientsList().length(); ++j)
        if (clientsUUIDsList.contains(this->getClientsList().at(j)->getUUID()))
            this->getClientsList().at(j)->socket->write(block);
}

QStringList ChatServer::getRegisteredClients() const
{
    QStringList regClientsList;
    QString strRegClientData;
    foreach (Client *client, clientsList)
        if (client->isRegistered())
        {
            strRegClientData = client->getName() + " " + client->getUUID();
            regClientsList << strRegClientData;
        }
    return regClientsList;
}

bool ChatServer::isNameUsed(QString name) const
{
    quint16 clientsQuantity = clientsList.length();
    for (int i = 0; i < clientsQuantity; ++i)
        if (QString::compare(clientsList.at(i)->getName(), name, Qt::CaseInsensitive) == 0)
            return true;
    return false;
}

bool ChatServer::clientExists(QString uuid) const
{
    quint16 clientsQuantity = clientsList.length();
    for (int i = 0; i < clientsQuantity; ++i)
        if ((clientsList.at(i)->getUUID() == uuid) && clientsList.at(i)->isRegistered())
            return true;
    return false;
}

bool ChatServer::isNameIllegal(QString name) const
{
    QString item;
    foreach (item, reservedNamesList)
        if (QString::compare(name, item, Qt::CaseInsensitive) == 0)
            return true;
    return false;
}

void ChatServer::sendCommandToAll(quint8 command)
{
    quint16 clientsQuantity = getClientsList().length();
    for (int i = 0; i < clientsQuantity; ++i)
        getClientsList().at(i)->sendCommand(command);
}

void ChatServer::deregisterAll()
{
    sendCommandToAll(Constants::comDeregisterClient);
    foreach (Client *item, getClientsList()) {
        emit removeClientFromGui(item->getUUID(), item->getName());
        item->setRegistered(false);
        item->setName("");
    }
}

void ChatServer::incomingConnection(qintptr handle)
{
    // create an client
    Client *client = new Client(handle, this, this);
    if (mainWindow != 0)
    {
        QObject::connect(client, SIGNAL(addClientToGui(QString,QString)), mainWindow, SLOT(onAddClientToGui(QString,QString)));
        QObject::connect(client, SIGNAL(removeClientFromGui(QString,QString)), mainWindow, SLOT(onRemoveClientFromGui(QString,QString)));
        QObject::connect(client, SIGNAL(messageToGui(QString,QString,QStringList)), mainWindow, SLOT(onMessageToGui(QString,QString,QStringList)));
    }
    QObject::connect(client, SIGNAL(removeClient(Client*)), this, SLOT(onRemoveClient(Client*)));
    QObject::connect(client, SIGNAL(stopChatServer()), mainWindow, SLOT(onStopChatServer()));
    QObject::connect(client, SIGNAL(addToLogArea(QString,bool)), mainWindow, SLOT(onAddToLogArea(QString,bool)));
    clientsList.append(client);
}

void ChatServer::onRemoveClient(Client *client)
{
    clientsList.removeAt(getClientsList().indexOf(client));
}

void ChatServer::sendMessageFromServer(QString message, const QStringList &clients)
{
    if (clients.isEmpty())
        sendToAllServerMessage(message);
    else
        sendServerMessageToClients(message, clients);
}

quint16 ChatServer::getRegisteredClientsQuantity()
{
    quint16 regClientsQuantity = 0;
    quint16 clientsQuantity = getClientsList().length();
    for (int i = 0; i < clientsQuantity; ++i)
    {
        if (getClientsList().at(i)->isRegistered())
            regClientsQuantity++;
    }
    return regClientsQuantity;
}

bool ChatServer::isCommandExpected(QString text)
{
    if (text[0] == '#')
        return true;
    return false;
}

void ChatServer::processCommand(QString text)
{
    addToLogArea(tr("<div style='color:red'>Unknown command: \"%1\" </div>").arg(text.left(text.indexOf(' '))));
}

void ChatServer::fillReservedNamesList()
{
    reservedNamesList.append("server");
    reservedNamesList.append("chatserver");
    reservedNamesList.append("majechatserver");
    reservedNamesList.append("client");
}

