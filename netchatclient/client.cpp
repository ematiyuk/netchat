#include <QtNetwork>
#include <QApplication>
#include <QMessageBox>
#include <QUuid>
#include <QMediaPlayer>
#include <QSound>
#include <QSoundEffect>
#include <ctime>
#include <algorithm>

#include "constants.h"
#include "utils.h"
#include "client.h"

Client::Client(QMainWindow *widget, QObject *parent) :
    QObject(parent), mainWindow(widget)
{
    uuid = generateUUID();

    socket = new QTcpSocket();

    connect(this, SIGNAL(addToLogArea(QString,bool)), mainWindow, SLOT(onAddToLogArea(QString,bool)));
    connect(this, SIGNAL(clearMessageArea()), mainWindow, SLOT(onClearMessageArea()));
    connect(this, SIGNAL(setWindowTitleWithClientName()), mainWindow, SLOT(onSetWindowTitleWithClientName()));
    connect(this, SIGNAL(adjustGUIOnDeregister()), mainWindow, SLOT(onAdjustGUIOnDeregister()));

    connect(this->getSocket(), SIGNAL(readyRead()), this, SLOT(onSocketReadyRead()));
    connect(this->getSocket(), SIGNAL(connected()), this, SLOT(onSocketConnected()));
    connect(this->getSocket(), SIGNAL(disconnected()), this, SLOT(onSocketDisconnected()));
    connect(this->getSocket(), SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(onSocketDisplayError(QAbstractSocket::SocketError)));
    connect(this, SIGNAL(showMessageInTray(QString,QString,QSystemTrayIcon::MessageIcon,int,bool)),
            mainWindow, SLOT(onShowMessageInTray(QString,QString,QSystemTrayIcon::MessageIcon,int,bool)));

    msgSound = new QMediaPlayer;
    msgSound->setMedia(QUrl("qrc:/data/msg.wav"));
    msgSound->setVolume(100);

    utils = new Utils();

}

QString Client::generateUUID()
{
    QUuid uuid = QUuid::createUuid();
    return uuid.toString();
}

QString Client::retrieveNameFromStr(QString str)
{
    return str.left(str.indexOf(" "));
}

void Client::onSocketDisplayError(QAbstractSocket::SocketError socketError)
{
    QApplication::alert(mainWindow);
    QString title = "Cannot connect to ChatServer";
    QString body = "An error occured while connecting.\nClick the message for details.";
    emit showMessageInTray(title, body, QSystemTrayIcon::Critical, 10000, false);

    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(mainWindow, "Connection Error - " + Constants::programName, "The host was not found");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(mainWindow, "Connection Error - " + Constants::programName, "The connection was refused by the peer.");
        break;
    default:
        QMessageBox::information(mainWindow, "Connection Error - " + Constants::programName, "The following error occurred: " + this->getSocket()->errorString());
    }
}

void Client::sendMessageToAll(QString message)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << (quint8)Constants::comMessageToAll << message;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    this->writeToSocket(block);
}

void Client::sendMessageToSelected(QString message, QString selectedClients)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << (quint8)Constants::comMessageToClients << selectedClients << message;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    this->writeToSocket(block);
}

void Client::onSocketReadyRead()
{
    QDataStream in(this->getSocket());
    // if read a new block so the first 2 bytes are its size
    if (blockSize == 0) {
        // if came less than 2 bytes, wait until 2 bytes come
        if (this->getSocket()->bytesAvailable() < (int)sizeof(quint16))
            return;
        // read size (2 bytes)
        in >> blockSize;
    }
    // wait until block comes completely
    if (this->getSocket()->bytesAvailable() < blockSize)
        return;
    else
        // one can accept a new block
        blockSize = 0;
    // the 3rd byte is a command to server
    quint8 command;
    in >> command;

    switch (command)
    {
    case Constants::comRegistrationSuccess:
    {
        emit addToLogArea("<div style='color:gray'>* Signed in as <b>" +
                          this->getName() + "</b></div>");
    }
        break;
    case Constants::comRegisteredClients:
    {
        QString clientsUUIDs;
        in >> clientsUUIDs;
        if (clientsUUIDs.isEmpty())
            return;
        QStringList clientsList = clientsUUIDs.split(",");
        emit addClientsToGUI(clientsList);
    }
        break;
    case Constants::comMessageToAll:
    {
        QString clientUUID;
        in >> clientUUID;
        QString clientName;
        in >> clientName;
        QString message;
        in >> message;
        QString strToLogArea;
        if (clientUUID == this->getUUID())
            strToLogArea = "<div style='color:blue'>[" +
                    QDateTime::currentDateTime().toString("MM/dd/yy h:mm:ss AP") +
                    "] From <b>Me</b> to all:</div>";
        else
        {
            strToLogArea = "<div style='color:navy'>[" +
                    QDateTime::currentDateTime().toString("MM/dd/yy h:mm:ss AP") +
                    "] <b>" + clientName + "</b> to all:</div>";
            QString title = Constants::programName;
            QString body = "[" + clientName + "] to all:\n" + utils->shortenForMessageInTray(message);
            emit showMessageInTray(title, body, QSystemTrayIcon::NoIcon, 5000);
            msgSound->play();
            QApplication::alert(mainWindow);
        }
        emit addToLogArea(strToLogArea, false);
        emit addToLogArea("<div style='color: black; white-space: pre-wrap;'>" + utils->replaceWebLinksInText(message) + "</div>");
    }
        break;
    case Constants::comMessageToClients:
    {
        QStringList receivers;
        in >> receivers;
        QString senderUUID;
        in >> senderUUID;
        QString senderName;
        in >> senderName;
        QString message;
        in >> message;

        QString strToLogArea;
        if (senderUUID == this->getUUID())
        {
            QStringList receiversNamesList;
            foreach (QString item, receivers) {
                receiversNamesList.append(this->retrieveNameFromStr(item));
            }
            strToLogArea = "<div style='color:orange'>[" +
                    QDateTime::currentDateTime().toString("MM/dd/yy h:mm:ss AP") +
                    "] From <b>Me</b> to <b>" + receiversNamesList.join(", ") + "</b>:</div>";
        }
        else
        {
            strToLogArea = "<div style='color:green'>[" +
                    QDateTime::currentDateTime().toString("MM/dd/yy h:mm:ss AP") +
                    "] <b>" + senderName + "</b>:</div>";
            QString title = Constants::programName;
            QString body = "[" + senderName + "]:\n" + utils->shortenForMessageInTray(message);
            emit showMessageInTray(title, body, QSystemTrayIcon::NoIcon, 5000);
            msgSound->play();
            QApplication::alert(mainWindow);
        }
        emit addToLogArea(strToLogArea, false);
        emit addToLogArea("<div style='color: black; white-space: pre-wrap;'>" + utils->replaceWebLinksInText(message) + "</div>");
    }
        break;
    case Constants::comPublicServerMessage:
    {
        QString message;
        in >> message;
        QString strToLogArea;
        QString title = Constants::programName;
        QString body = "ChatServer (broadcast):\n" + utils->shortenForMessageInTray(message);
        emit showMessageInTray(title, body, QSystemTrayIcon::NoIcon, 5000);
        msgSound->play();
        QApplication::alert(mainWindow);
        strToLogArea = "<div style='color:black'><b>ChatServer</b> (broadcast):</div>";
        emit addToLogArea(strToLogArea, false);
        emit addToLogArea("<div style='color: black; white-space: pre-wrap;'>" + utils->replaceWebLinksInText(message) + "</div>");
    }
        break;
    case Constants::comPrivateServerMessage:
    {
        QString message;
        in >> message;
        QString strToLogArea;
        QString title = Constants::programName;
        QString body = "ChatServer (private):\n" + utils->shortenForMessageInTray(message);
        emit showMessageInTray(title, body, QSystemTrayIcon::NoIcon, 5000);
        msgSound->play();
        QApplication::alert(mainWindow);
        strToLogArea = "<div style='color:blue'><b>ChatServer</b> (private):</div>";
        emit addToLogArea(strToLogArea, false);
        emit addToLogArea("<div style='color: black; white-space: pre-wrap;'>" + utils->replaceWebLinksInText(message) + "</div>");
    }
        break;
    case Constants::comClientJoined:
    {
        QString uuid;
        in >> uuid;
        QString name;
        in >> name;
        emit addClientToGUI(uuid, name);
    }
        break;
    case Constants::comClientLeft:
    {
        QString uuid;
        in >> uuid;
        QString name;
        in >> name;
        emit removeClientFromGUI(uuid, name);
    }
        break;
    case Constants::comDisconnectClient:
    {
        this->disconnectFromChatServer();
        QString title = "Disconnected from ChatServer";
        QString body = "Server has been stopped.";
        emit showMessageInTray(title, body, QSystemTrayIcon::Information, 10000, false);
        QApplication::alert(mainWindow);
        QMessageBox::information(mainWindow, Constants::programName, "You have been disconnected.\nChatServer has been stopped.");
    }
        break;
    case Constants::comDeregisterClient:
    {
        emit adjustGUIOnDeregister();
    }
        break;
    case Constants::comErrClientExists:
    {
        QApplication::alert(mainWindow);
        QMessageBox::warning(mainWindow, "Authorization Error - " + Constants::programName, "You are already authorized.\\nBut now you are not :) Bye.");
        this->disconnectFromChatServer();
    }
        break;
    case Constants::comErrNameInvalid:
    {
        QApplication::alert(mainWindow);
        QMessageBox::warning(mainWindow, "Authorization Error - " + Constants::programName, "Client name is invalid.");
        this->disconnectFromChatServer();
    }
        break;
    case Constants::comErrNameUsed:
    {
        QApplication::alert(mainWindow);
        QMessageBox::warning(mainWindow, "Authorization Error - " + Constants::programName, "Client name is already used.");
        this->disconnectFromChatServer();
    }
        break;
    case Constants::comErrNameIllegal:
    {
        QApplication::alert(mainWindow);
        QMessageBox::warning(mainWindow, "Authorization Error - " + Constants::programName, "Client name is illegal.");
        this->disconnectFromChatServer();
    }
        break;
    }
}

bool Client::isCommandExpected(QString text)
{
    if (text[0] == '#')
        return true;
    return false;
}

void Client::processCommand(QString text)
{
    QRegExp pingCommandRegExp("^ping$");
    pingCommandRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    if (pingCommandRegExp.indexIn(text) != -1)
    {
        // ping command
        this->sendCommand(Constants::comPing);
        emit clearMessageArea();
    }
    else
    {
        addToLogArea(tr("<div style='color:red'>Unknown command: \"%1\" </div>").arg(text.left(text.indexOf(' '))));
    }
}

bool Client::isConnected()
{
    if (this->getSocket()->state() == QAbstractSocket::ConnectedState)
        return true;
    else
        return false;
}

bool Client::connectToChatServer(QHostAddress ipAddress, quint16 port)
{
    this->getSocket()->connectToHost(ipAddress, port);
    if (!this->getSocket()->waitForConnected(3000))
        return false;
    else
        return true;
}

void Client::disconnectFromChatServer()
{
    this->getSocket()->disconnectFromHost();
}

void Client::tryToRegister(QString name)
{
    blockSize = 0;
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << (quint8)Constants::comRegisterRequest;
    out << this->getUUID();
    out << name;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    this->writeToSocket(block);
}

void Client::sendCommand(quint8 command)
{
    blockSize = 0;
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << (quint8)command;
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    this->writeToSocket(block);
}

void Client::sendClientConnected()
{
    blockSize = 0;
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << (quint16)0;
    out << (quint8)Constants::comClientConnected;
    out << this->getUUID();
    out.device()->seek(0);
    out << (quint16)(block.size() - sizeof(quint16));
    this->writeToSocket(block);
}

void Client::writeToSocket(QByteArray block)
{
    this->getSocket()->write(block);
}

void Client::onSocketConnected()
{
    this->sendClientConnected();
}

void Client::onSocketDisconnected()
{
    emit clientDisconnected();
}
