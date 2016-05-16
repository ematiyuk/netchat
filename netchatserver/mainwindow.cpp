#include <QMessageBox>
#include <QScrollBar>
#include <QMenu>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "server.h"
#include "constants.h"
#include "utils.h"

MainWindow::MainWindow(QMainWindow *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->pteMessage->installEventFilter(this);

    chatServer = new ChatServer(this, this);
    utils = new Utils();

    createActions();
    createTrayIcon();

    QObject::connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                     this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    setIcon();
    trayIcon->show();

    this->setDefaults();
    this->startChatServerIfNecessary();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setVisible(bool visible)
{
    minimizeAction->setEnabled(visible);
    restoreAction->setEnabled(isMinimized() || !visible);
    QMainWindow::setVisible(visible);
}

void MainWindow::setIcon()
{
    QIcon icon(":/data/icon.png");
    trayIcon->setIcon(icon);
    setWindowIcon(icon);
}

void MainWindow::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason) {
    case QSystemTrayIcon::Trigger:
    case QSystemTrayIcon::DoubleClick:
    case QSystemTrayIcon::MiddleClick:
        this->activateWindow();
        this->showNormal();
        break;
    default:
        ;
    }
}

void MainWindow::createActions()
{
    startAction = new QAction("Start", this);
    connect(startAction, SIGNAL(triggered()), this, SLOT(on_pbStart_clicked()));

    stopAction = new QAction("Stop", this);
    connect(stopAction, SIGNAL(triggered()), this, SLOT(on_pbStop_clicked()));

    minimizeAction = new QAction("Minimize", this);
    connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

    restoreAction = new QAction("Restore", this);
    connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

    quitAction = new QAction("Quit", this);
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
}

void MainWindow::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(startAction);
    trayIconMenu->addAction(stopAction);
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
}

bool MainWindow::eventFilter(QObject *object, QEvent *event)
{
    if (object == ui->pteMessage && event->type() == QEvent::KeyPress)
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        // Check if Ctrl+Enter is pressed
        if (keyEvent->key() == Qt::Key_Return && keyEvent->modifiers().testFlag(Qt::ControlModifier))
        {
            this->tryToSendMessage();
            return true;
        }
        else
        {
            return QWidget::eventFilter(object, event);
        }
    }
    else
    {
        return QWidget::eventFilter(object, event);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (trayIcon->isVisible())
    {
        hide();
        event->ignore();
    }
    else
    {
        if (chatServer->isListening())
        {
            if (!chatServer->getClientsList().isEmpty())
            {
                QString text = "Are you sure you want to stop the ChatServer?\n"
                        "All active users will be signed out and disconnected as well.";
                int ret = QMessageBox::question(this, "Prompt", text, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
                if (ret == QMessageBox::Yes)
                {
                    this->onStopChatServer();
                    event->accept();
                }
                else
                {
                    event->ignore();
                }
            }
        }
    }
}


void MainWindow::tryToStartChatServer()
{
    QString addressFromWidget = ui->leHost->text();
    QString portFromWidget = ui->sbPort->text();
    if (chatServer->startChatServer(QHostAddress(addressFromWidget), portFromWidget.toInt()))
    {
        QString strToLogArea = "<div style='color:gray'>[" +
                QDateTime::currentDateTime().toString("MM/dd/yy h:mm:ss AP") +
                "] ChatServer started at " + chatServer->serverAddress().toString() +
                ":" + QString::number(chatServer->serverPort()) + "</div>";
        this->addToLogArea(strToLogArea);
        this->setWindowTitle(Constants::programName + " - " + addressFromWidget + ":" + portFromWidget + " - [Running]");
        trayIcon->setToolTip(Constants::programName + " (" + addressFromWidget + ":" + portFromWidget + ")");
        ui->pbStart->setDisabled(true);
        ui->pbStop->setEnabled(true);
        startAction->setDisabled(true);
        stopAction->setEnabled(true);

        ui->pbSend->setEnabled(true);
        ui->cbToAll->setEnabled(true);
        ui->pteMessage->setEnabled(true);
        ui->leHost->setReadOnly(true);
        ui->sbPort->setReadOnly(true);
        ui->pbStop->setText("Stop");
        ui->pteMessage->setFocus();
    }
    else
    {
        QString strToLogArea = "<div style='color:red'>[" +
                QDateTime::currentDateTime().toString("MM/dd/yy h:mm:ss AP") +
                "] ChatServer failed to start:" + chatServer->errorString() + "</div>";
        this->addToLogArea(strToLogArea);
        this->setWindowTitle(Constants::programName + " - [Not started]");
        trayIcon->setToolTip(Constants::programName + " (Not started)");
        this->adjustGUIOnServerStopped();
    }
}

void MainWindow::startChatServerIfNecessary()
{
    if (ui->cbAutostart->isChecked())
    {
        ui->pbStart->setDisabled(true);
        startAction->setDisabled(true);

        ui->leHost->setReadOnly(true);
        ui->sbPort->setReadOnly(true);
        ui->pbStop->setText("Starting...");
        QTimer::singleShot(3 * 1000, this, SLOT(tryToStartChatServer()));
    }
}

QVariant MainWindow::loadOneSetting(const QString &key, const QVariant &defaultValue)
{
    QSettings settings("MaJe org", Constants::programName);
    return settings.value(key, defaultValue);
}

void MainWindow::saveOneSetting(const QString &key, const QVariant &value)
{
    QSettings settings("MaJe org", Constants::programName);
    settings.setValue(key, value);
}

void MainWindow::onAddClientToGui(QString uuid, QString name)
{
    ui->lwClients->addItem(name + " " + uuid);
    QString strToLogArea = "<div style='color:gray'>* User <b>" + name + " " + uuid + "</b> has signed in</div>";
    this->addToLogArea(strToLogArea);
}

QString MainWindow::retrieveUUIDFromStr(QString str)
{
    return str.remove(0, str.indexOf('{'));
}

void MainWindow::onRemoveClientFromGui(const QString &uuid, const QString &name)
{
    if (chatServer->isListening())
    {
        quint16 clientsQuantity = ui->lwClients->count();
        for (int i = 0; i < clientsQuantity; ++i)
            if (retrieveUUIDFromStr(ui->lwClients->item(i)->text()) == uuid)
            {
                ui->lwClients->takeItem(i);
                this->addToLogArea("<div style='color:gray'>* User <b>" + name + " " + uuid +
                                   "</b> has signed out</div>");
                break;
            }
    }
}

QString MainWindow::retrieveNameFromStr(QString str)
{
    return str.left(str.indexOf(" "));
}

void MainWindow::onMessageToGui(QString message, QString senderClientName, const QStringList &receiversList)
{
    if (receiversList.isEmpty())
    {
        QString strToLogArea = "<div style='color:orange'>[" + QDateTime::currentDateTime().toString("MM/dd/yy h:mm:ss AP") +
                "] Message from <b>" + senderClientName + "</b> to all:</div>";
        this->addToLogArea(strToLogArea, false);
        this->addToLogArea("<div style='color: black; white-space: pre-wrap;'>" + utils->replaceWebLinksInText(message) + "</div>");
    }
    else
    {
        QStringList receiversNamesList;
        foreach (QString item, receiversList) {
            receiversNamesList.append(this->retrieveNameFromStr(item));
        }
        QString strToLogArea = "<div style='color:green'>[" + QDateTime::currentDateTime().toString("MM/dd/yy h:mm:ss AP") +
                "] Message from <b>" + senderClientName + "</b> to <b>" + receiversNamesList.join(", ") + "</b>:</div>";
        this->addToLogArea(strToLogArea, false);
        this->addToLogArea("<div style='color: black; white-space: pre-wrap;'>" + utils->replaceWebLinksInText(message) + "</div>");
    }
}

void MainWindow::onAddToLogArea(const QString &text, bool emptyLineIsNeeded)
{
    addToLogArea(text, emptyLineIsNeeded);
}

void MainWindow::addToLogArea(const QString &text, bool emptyLineIsNeeded)
{
    if (emptyLineIsNeeded)
    {
        ui->teLogArea->append(text);
        ui->teLogArea->append("");
    }
    else
    {
        ui->teLogArea->append(text);
    }
    ui->teLogArea->verticalScrollBar()->setValue(ui->teLogArea->verticalScrollBar()->maximum());
}

void MainWindow::onClearMessageArea()
{
    ui->pteMessage->clear();
}

void MainWindow::tryToSendMessage()
{
    QString textFromMessageField = ui->pteMessage->document()->toPlainText().trimmed();
    if (textFromMessageField.isEmpty())
        return;
    if (chatServer->isCommandExpected(textFromMessageField))
    {
        chatServer->processCommand(textFromMessageField.remove(0, 1));
    }
    else
    {
        if (ui->lwClients->count() == 0)
        {
            QMessageBox::warning(this, Constants::programName, "Cannot send a message.\nNo users are authorized.");
            return;
        }
        QStringList selectedClients;
        QString strToLogArea;
        if (!ui->cbToAll->isChecked())
        {
            if (ui->lwClients->selectedItems().isEmpty())
            {
                QMessageBox::warning(this, Constants::programName, "Cannot send a message.\nNo users are selected.");
                return;
            }
            foreach (QListWidgetItem *item, ui->lwClients->selectedItems())
                selectedClients << item->text();
            QStringList selectedClientsNamesList;
            foreach (QString item, selectedClients) {
                selectedClientsNamesList.append(this->retrieveNameFromStr(item));
            }

            strToLogArea = "<div style='color:blue'><b>ChatServer</b> to <b>" + selectedClientsNamesList.join(", ") + "</b>:</div>";
        }
        else
        {
            strToLogArea = "<div style='color:navy'> <b>ChatServer</b> to all:</div>";
        }
        chatServer->sendMessageFromServer(textFromMessageField, selectedClients);
        addToLogArea(strToLogArea, false);
        addToLogArea("<div style='color: black; white-space: pre-wrap;'>" +
                     utils->replaceWebLinksInText(textFromMessageField) + "</div>");
        ui->pteMessage->clear();
        ui->pteMessage->setFocus();
    }
}

void MainWindow::on_pbSend_clicked()
{
    this->tryToSendMessage();
}

void MainWindow::adjustGUIOnServerStopped()
{
    ui->pbStop->setDisabled(true);
    ui->pbStart->setEnabled(true);
    stopAction->setDisabled(true);
    startAction->setEnabled(true);

    ui->pbSend->setDisabled(true);
    ui->cbToAll->setDisabled(true);
    ui->pteMessage->setDisabled(true);
    ui->leHost->setReadOnly(false);
    ui->sbPort->setReadOnly(false);
    ui->pbStart->setFocus();
    ui->lwClients->clear();
    ui->pteMessage->clear();
    ui->cbToAll->setChecked(true);
    this->on_cbToAll_clicked();
}

void MainWindow::on_cbToAll_clicked()
{
    if (ui->cbToAll->isChecked())
    {
        ui->pbSend->setText("Send to all");
        ui->lwClients->setDisabled(true);
        ui->lwClients->clearSelection();
    }
    else
    {
        ui->pbSend->setText("Send to selected");
        ui->lwClients->setEnabled(true);
    }
}

void MainWindow::on_pbStart_clicked()
{
    QHostAddress ipAddress;
    QRegExp ipAddressRegExp("^(([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])\\.){3}([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$");

    QString addressFromWidget = ui->leHost->text();
    QString portFromWidget = ui->sbPort->text();
    if (!ipAddressRegExp.exactMatch(addressFromWidget) || !ipAddress.setAddress(addressFromWidget))
    {
        QMessageBox::warning(this, "Input error", "Invalid entered IP address.\nPlease try again.");
        ui->leHost->setReadOnly(false);
        ui->sbPort->setReadOnly(false);
        ui->leHost->setFocus();
        return;
    }
    this->tryToStartChatServer();

    this->saveOneSetting("hostValue", addressFromWidget);
    this->saveOneSetting("portValue", portFromWidget);
}

void MainWindow::onStopChatServer()
{
    QString strToLogArea = "<div style='color:gray'>[" +
            QDateTime::currentDateTime().toString("MM/dd/yy h:mm:ss AP") +
            "] ChatServer stopped</div>";
    this->addToLogArea(strToLogArea);
    this->setWindowTitle(Constants::programName + " - [Not started]");
    trayIcon->setToolTip(Constants::programName + " (Not started)");

    chatServer->sendCommandToAll(Constants::comDisconnectClient);  // send to all users a disconnect command
    chatServer->close();    // stop listening for new connections

    this->adjustGUIOnServerStopped();
}

void MainWindow::on_pbStop_clicked()
{
    if (!chatServer->getClientsList().isEmpty())
    {
        QString text = "Are you sure you want to stop the ChatServer?\n"
                "All active users will be signed out and disconnected as well.";
        int ret = QMessageBox::question(this, "Prompt", text, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret != QMessageBox::Yes)
        {
            return;
        }
    }
    this->onStopChatServer();
}

void MainWindow::setDefaults()
{
    this->setWindowTitle(Constants::programName + " - [Not started]");
    trayIcon->setToolTip(Constants::programName + " (Not started)");
    this->setFixedSize(723, 585);
    ui->teLogArea->setReadOnly(true);
    ui->pbStop->setDisabled(true);
    ui->pbStart->setEnabled(true);
    stopAction->setDisabled(true);
    startAction->setEnabled(true);
    ui->pbSend->setDisabled(true);
    ui->cbToAll->setDisabled(true);
    ui->pteMessage->setDisabled(true);
    ui->lwClients->setDisabled(true);
    ui->pbStart->setFocus();

    ui->leHost->setText(this->loadOneSetting("hostValue", "127.0.0.1").toString());
    ui->sbPort->setValue(this->loadOneSetting("portValue", 1616).toInt());
    ui->cbAutostart->setChecked(this->loadOneSetting("autoStartState", false).toBool());
}

void MainWindow::on_cbAutostart_toggled(bool checked)
{
    this->saveOneSetting("autoStartState", checked);
}
