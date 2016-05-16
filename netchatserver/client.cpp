#include <QWidget>
#include <QMessageBox>

#include "client.h"
#include "constants.h"

Client::Client(qintptr socketDesc, ChatServer *chatServerPtr, QObject *parent) : QObject(parent), socketDescriptor(socketDesc)
{
    qRegisterMetaType<QAbstractSocket::SocketError>();
    // holds a pointer on blackboard-object
    chatServer = chatServerPtr;
    // a client didn't pass registration
    this->setName(Constants::constNameUnknown);
    this->setUUID("{00000000-0000-0000-0000-000000000000}");
    this->setRegistered(false);
    // receivable block size = 0
    blockSize = 0;
    // create a socket
    socket = new QTcpSocket(this);
    // set the descriptor from incomingConnection()
    socket->setSocketDescriptor(socketDescriptor);

    utils = new Utils();

    // connect signals
    connect(socket, SIGNAL(connected()), this, SLOT(onConnect()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(onDisconnect()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(onError(QAbstractSocket::SocketError)));
}

Client::~Client()
{
}

void Client::onConnect()
{
    // never calls, socket is already connected to the tcpserver
    // we are just binding to this socket here: socket->setSocketDescriptor(socketDesciptor);
}

void Client::onDisconnect()
{
    // if an client is registered
    if (isRegistered())
    {
        // remove from GUI
        emit removeClientFromGui(this->getUUID(), this->getName());
        // tell everyone that an client has left
        chatServer->sendToAllHasLeft(this->getUUID(), this->getName());
        // remove from clients list
        emit removeClient(this);
    }
    else
    {
        // remove from clients list
        emit removeClient(this);
    }
    emit addToLogArea("<div style='color:gray'>* User <b>" + this->getUUID() + "</b> has disconnected</div>");

    socket->close();
    socket->deleteLater();
    this->deleteLater();
}

void Client::onError(QAbstractSocket::SocketError socketError) const
{
    // w is needed for memory deallocation from QMessageBox (with the help of *parent = &w)
    QWidget w;
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(&w, "Error", "The host was not found");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(&w, "Error", "The connection was refused by the peer.");
        break;
    default:
        QMessageBox::information(&w, "Error", "The following error occurred: " + socket->errorString());
    }
    // here will be called desctructor for w and accordingly QMessageBox (according to C++ rules)
}

void Client::onReadyRead()
{
    QDataStream in(socket);
    // if read a new block so the first 2 bytes are its size
    if (blockSize == 0) {
        // if came less than 2 bytes, wait until 2 bytes come
        if (socket->bytesAvailable() < (int)sizeof(quint16))
            return;
        // read size (2 bytes)
        in >> blockSize;
    }
    // wait until block comes completely
    if (socket->bytesAvailable() < blockSize)
        return;
    else
        // one can accept a new block
        blockSize = 0;
    // the 3rd byte is a command to server
    quint8 command;
    in >> command;
    // for unregistered clients accepts command "registration request" only
    if (!this->isRegistered() && command != Constants::comRegisterRequest
            && command != Constants::comClientConnected
            && command != Constants::comPing)
        return;

    switch(command)
    {
    // request for registration
    case Constants::comRegisterRequest:
    {
        // read client data
        QString uuidFromStream;
        in >> uuidFromStream;
        QString nameFromStream;
        in >> nameFromStream;

        // check whether client exists already
        if (chatServer->clientExists(uuidFromStream))
        {
            // send an error
            chatServer->sendCommand(Constants::comErrClientExists, this->getUUID());
            emit removeClient(this);
            return;
        }
        // check whether name is valid
        if (!utils->isNameValid(nameFromStream))
        {
            // send an error
            chatServer->sendCommand(Constants::comErrNameInvalid, this->getUUID());
            emit removeClient(this);
            return;
        }
        // check whether name is illegal
        if (chatServer->isNameIllegal(nameFromStream))
        {
            // send an error
            chatServer->sendCommand(Constants::comErrNameIllegal, this->getUUID());
            emit removeClient(this);
            return;
        }
        // check whether name is used already
        if (chatServer->isNameUsed(nameFromStream))
        {
            // send an error
            chatServer->sendCommand(Constants::comErrNameUsed, this->getUUID());
            emit removeClient(this);
            return;
        }

        // registration success
        this->setUUID(uuidFromStream);
        this->setName(nameFromStream);
        this->setRegistered(true);

        // send to the new client a list of active clients
        sendRegisteredClients();
        // add to GUI
        emit addClientToGui(this->getUUID(), this->getName());
        // inform everyone about new client
        chatServer->sendToAllHasJoined(this->getUUID(), this->getName());
        // inform the client about success
        //if (socket->waitForBytesWritten())
        //    sendCommand(comRegistrationSuccess);
    }
        break;
        // request for deregistration
    case Constants::comDeregisterRequest:
    {
        this->setRegistered(false);
        emit removeClientFromGui(this->getUUID(), this->getName());
        chatServer->sendToAllHasLeft(this->getUUID(), this->getName());
        this->setName("");
    }
        break;
    case Constants::comClientConnected:
    {
        QString uuidFromStream;
        in >> uuidFromStream;
        this->setUUID(uuidFromStream);
        emit addToLogArea("<div style='color:gray'>* User <b>" + this->getUUID() + "</b> has connected</div>");
    }
        break;
        // a message to all has come from current client
    case Constants::comMessageToAll:
    {
        QString message;
        in >> message;
        // send this message to all clients
        chatServer->sendToAllMessage(message, this->getUUID(), this->getName());
        // update log area of the server
        emit messageToGui(message, this->getName(), QStringList());
    }
        break;
        // a message for several clients has come from current client
    case Constants::comMessageToClients:
    {
        QString clientsReceivers;
        in >> clientsReceivers;
        QString message;
        in >> message;
        // split a string on the names with UUIDs
        QStringList clients = clientsReceivers.split(",");
        // send this message to necessary clients
        chatServer->sendMessageToClients(message, clients, this->getUUID(), this->getName());
        // update log area
        emit messageToGui(message, this->getName(), clients);
    }
        break;
    case Constants::comPing:
    {
        chatServer->sendServerMessageToClients("<div style='color:gray;'>pong</b>", QStringList(this->getUUID()));
    }
        break;
    }
}

void Client::sendCommand(quint8 comm) const
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << comm;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    socket->write(block);
}

void Client::sendRegisteredClients() const
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << Constants::comRegisteredClients;
    QStringList clientsList = chatServer->getRegisteredClients();
    QString clientsStr;
    quint16 clientsQuantity = clientsList.length();
    for (int i = 0; i < clientsQuantity; ++i)
        if (clientsList.at(i) != (this->getName() + " " + this->getUUID()))
            clientsStr += clientsList.at(i) + (QString)",";
    clientsStr.remove(clientsStr.length()-1, 1);
    out << clientsStr;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint32));
    socket->write(block);
}
