#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "client.h"
#include "constants.h"

#include <QtNetwork>
#include <QtGui>
#include <QDebug>
#include <QMessageBox>
#include <QScrollBar>
#include <QAction>
#include <QMenu>
#include <QStandardPaths>

MainWindow::MainWindow(QMainWindow *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->pteMessage->installEventFilter(this);

    client = new Client(this, this);

    createActions();
    createTrayIcon();

    QObject::connect(client, SIGNAL(clientDisconnected()), this, SLOT(onClientDisconnected()));
    QObject::connect(client, SIGNAL(addClientsToGUI(QStringList)), this, SLOT(onAddClientsToGUI(QStringList)));
    QObject::connect(client, SIGNAL(addClientToGUI(QString,QString)), this, SLOT(onAddClientToGUI(QString,QString)));
    QObject::connect(client, SIGNAL(removeClientFromGUI(QString,QString)), this, SLOT(onRemoveClientFromGUI(QString,QString)));

    QObject::connect(trayIcon, SIGNAL(messageClicked()), this, SLOT(onMessageClicked()));
    QObject::connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                     this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));

    setIcon();
    trayIcon->show();

    this->setDefaults();
    this->signInIfNecessary();
    loadHistory();
}

MainWindow::~MainWindow()
{
    saveHistory();
    delete ui;
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

void MainWindow::openMainWindow()
{
    if (this->isMinimized())
    {
        this->showNormal();
    }
    else
    {
        this->activateWindow();
    }
}

void MainWindow::onMessageClicked()
{
    this->activateWindow();
    this->showNormal();
}

void MainWindow::createActions()
{
    openAction = new QAction("Open", this);
    connect(openAction, SIGNAL(triggered()), this, SLOT(openMainWindow()));

    quitAction = new QAction("Quit", this);
    connect(quitAction, SIGNAL(triggered()), this, SLOT(close()));
}

void MainWindow::createTrayIcon()
{
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(openAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);

    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
}

void MainWindow::onShowMessageInTray(const QString &title, const QString &msg,
                                     QSystemTrayIcon::MessageIcon icon,
                                     int msecs, bool isMsg = true)
{
    if (isMsg)
    {
        if (this->isMinimized() || !this->isActiveWindow())
            trayIcon->showMessage(title, msg, icon, msecs);
    }
    else
    {
        trayIcon->showMessage(title, msg, icon, msecs);
    }
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
            return QMainWindow::eventFilter(object, event);
        }
    }
    else
    {
        return QMainWindow::eventFilter(object, event);
    }
}


void MainWindow::closeEvent(QCloseEvent *event)
{
    if(client->isConnected())
    {
        QString text = "Are you sure you want to disconnect from the Server?\n"
                "You will be disconnected and signed out as well.";
        int ret = QMessageBox::question(this, "Prompt", text, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes)
        {
            client->disconnectFromChatServer();
            event->accept();
        }
        else
        {
            event->ignore();
        }
    }
}

void MainWindow::signIn()
{
    if (ui->leName->text().isEmpty())
    {
        ui->pbSignInOut->setChecked(false);
        return;
    }
    client->tryToRegister(ui->leName->text());
    client->setName(ui->leName->text());

    ui->leName->setReadOnly(true);
    ui->pbSignInOut->setText("Sign out");
    ui->pbSignInOut->setEnabled(true);
    ui->pbSignInOut->setChecked(true);
    ui->cbToAll->setChecked(false);
    ui->pteMessage->setFocus();

    onSetWindowTitleWithClientName();
    onClearMessageArea();
}

void MainWindow::signInIfNecessary()
{
    if (ui->cbAutoSignIn->isChecked())
    {
        this->tryToConnectToChatServer();
        if (client->isConnected())
        {
            ui->pbSignInOut->setDisabled(true);
            ui->leName->setReadOnly(true);
            ui->pbSignInOut->setText("Signing in...");
            QTimer::singleShot(2 * 1000, this, SLOT(signIn()));
        }
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

QString MainWindow::prepareForSaving(QString text)
{
    QString textToFind = "<span style=\" color:#ffffff;\">-- TO APPEND --</span>";
    int textToFindLength = textToFind.length();
    int textToFindIndex = text.lastIndexOf(textToFind);
    QString textPreparingToSave = text.right(text.length() - textToFindIndex);
    QString textToSave = textPreparingToSave.remove(0, textToFindLength);
    QString textWithEndTags = "</p></body></html>";
    if (textToSave.startsWith(textWithEndTags))
    {
        int endTagsTextLength = textWithEndTags.length();
        int endTagsTextIndex = textToSave.indexOf(textWithEndTags);
        textToSave = textToSave.remove(endTagsTextIndex, endTagsTextLength);
    }
    else if (textToSave.startsWith(textWithEndTags = "</p>"))
    {
        int endTagsLength = textWithEndTags.length();
        int endTagsIndex = textToSave.indexOf(textWithEndTags);
        textToSave = textToSave.remove(endTagsIndex, endTagsLength);
    }

    return textToSave;
}

void MainWindow::saveHistory()
{
    QString historyDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + "/hist/";
    if (!QDir(historyDir).exists())
        QDir().mkpath(historyDir);
    QFile file(historyDir + "msg" + QDate::currentDate().toString("MMddyy") + ".hist");
    if (!file.open(QIODevice::Append | QIODevice::Text))
        return;

    QTextStream out(&file);
    out << prepareForSaving(ui->teLogArea->document()->toHtml());
    file.close();
}

void MainWindow::loadHistory()
{
    QFile file(QStandardPaths::writableLocation(QStandardPaths::DataLocation) +
               "/hist/msg" + QDate::currentDate().toString("MMddyy") + ".hist");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream out(&file);
    ui->teLogArea->insertHtml(out.readAll());
    file.close();
    ui->teLogArea->append("<span style=\"color:#ffffff;\">-- TO APPEND --</span>");
}

// slot
void MainWindow::onAddToLogArea(const QString &text, bool emptyLineIsNeeded)
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

void MainWindow::onAddClientsToGUI(const QStringList &clientsList)
{
    ui->lwClients->addItems(clientsList);
}

void MainWindow::onAddClientToGUI(const QString &uuid, const QString &name)
{
    ui->lwClients->addItem(name + " " + uuid);
    QString title = Constants::programName;
    QString body = "[" + name + "]\nis online";
    trayIcon->showMessage(title, body, QSystemTrayIcon::NoIcon, 5000);
}

void MainWindow::onClearMessageArea()
{
    ui->pteMessage->clear();
}

void MainWindow::onSetWindowTitleWithClientName()
{
    // set new window title with authorized client's name
    QString newWindowTitle = this->windowTitle().left(this->windowTitle().indexOf("[")) + client->getName();
    trayIcon->setToolTip(Constants::programName + " (" + client->getName() + ")");
    this->setWindowTitle(newWindowTitle);
}

void MainWindow::onSetWindowTitleNoAuth()
{
    QString newWindowTitle = this->windowTitle().replace(client->getName(), "Not authorized");
    trayIcon->setToolTip(Constants::programName + " (Not authorized)");
    this->setWindowTitle(newWindowTitle);
}

void MainWindow::onAdjustGUIOnDeregister()
{
    this->setWindowTitle(Constants::programName + " - [Not authorized]");
    ui->lwClients->clear();
    ui->cbToAll->setChecked(true);
}

QString MainWindow::retrieveUUIDFromStr(QString str)
{
    return str.remove(0, str.indexOf('{'));
}

void MainWindow::onRemoveClientFromGUI(const QString &uuid, const QString &name)
{
    quint16 clientsQuantity = ui->lwClients->count();
    for (int i = 0; i < clientsQuantity; ++i)
        if (retrieveUUIDFromStr(ui->lwClients->item(i)->text()) == uuid)
        {
            ui->lwClients->takeItem(i);
            QString title = Constants::programName;
            QString body = "[" + name + "]\nwent offline";
            trayIcon->showMessage(title, body, QSystemTrayIcon::NoIcon, 5000);
            break;
        }
}

void MainWindow::onClientDisconnected()
{
    QString title = "Disconnected from ChatServer";
    QString body = "You have been disconnected successfully.";
    trayIcon->showMessage(title, body, QSystemTrayIcon::Information, 10000);
    this->setWindowTitle(Constants::programName + " - [Not connected]");
    trayIcon->setToolTip(Constants::programName + " (Not connected)");
    this->adjustGUIOnClientDisconnected();
}

void MainWindow::adjustGUIOnClientDisconnected(){
    ui->pbConnect->setEnabled(true);
    ui->pbDisconnect->setDisabled(true);
    ui->pbSend->setDisabled(true);
    ui->cbToAll->setDisabled(true);
    ui->pteMessage->setDisabled(true);
    ui->leHost->setReadOnly(false);
    ui->sbPort->setReadOnly(false);
    ui->pbConnect->setFocus();
    ui->lwClients->clear();
    ui->pteMessage->clear();
    ui->cbToAll->setChecked(true);
    ui->leName->setReadOnly(true);
    ui->pbSignInOut->setDisabled(true);
    ui->pbSignInOut->setChecked(false);
    ui->pbSignInOut->setText("Sign in");
    this->on_cbToAll_toggled(true);
}

void MainWindow::tryToConnectToChatServer()
{
    QString addressFromWidget = ui->leHost->text();
    QString portFromWidget = ui->sbPort->text();
    if (client->connectToChatServer(QHostAddress(addressFromWidget), portFromWidget.toShort()))
    {
        this->setWindowTitle(Constants::programName + " - [Not authorized]");
        trayIcon->setToolTip(Constants::programName + " (Not authorized)");
        ui->pbConnect->setDisabled(true);
        ui->pbDisconnect->setEnabled(true);
        ui->pbSend->setEnabled(true);
        ui->cbToAll->setEnabled(true);
        ui->pteMessage->setEnabled(true);
        ui->leHost->setReadOnly(true);
        ui->sbPort->setReadOnly(true);
        ui->leName->setReadOnly(false);
        ui->pbSignInOut->setEnabled(true);
        ui->pbSignInOut->setChecked(false);
        ui->leName->setFocus();
    }
    else
    {
        this->setWindowTitle(Constants::programName + " - [Not connected]");
        trayIcon->setToolTip(Constants::programName + " (Not connected)");
        this->adjustGUIOnClientDisconnected();
    }
}

void MainWindow::on_pbConnect_clicked()
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
    this->tryToConnectToChatServer();

    this->saveOneSetting("hostValue", addressFromWidget);
    this->saveOneSetting("portValue", portFromWidget);
}

void MainWindow::on_pbDisconnect_clicked()
{
    if(client->isConnected())
    {
        QString text = "Are you sure you want to disconnect from the Server?\n"
                "You will be disconnected and signed out as well.";
        int ret = QMessageBox::question(this, "Prompt", text, QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (ret == QMessageBox::Yes)
        {
            this->setWindowTitle(Constants::programName + " - [Not connected]");
            trayIcon->setToolTip(Constants::programName + " (Not connected)");
            client->disconnectFromChatServer();
        }
    }
}

void MainWindow::on_cbToAll_toggled(bool checked)
{
    if (checked)
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

void MainWindow::tryToSendMessage()
{
    QString textFromMessageField = ui->pteMessage->document()->toPlainText().trimmed();
    if (textFromMessageField.isEmpty())
        return;
    if (client->isCommandExpected(textFromMessageField))
    {
        client->processCommand(textFromMessageField.remove(0, 1));
    }
    else
    {
        if (ui->cbToAll->isChecked())
        {
            if (ui->lwClients->count() == 0)
            {
                QMessageBox::warning(this, Constants::programName, "Cannot send a message.\nNo users are online or you are not authorized.");
                return;
            }
            client->sendMessageToAll(textFromMessageField);
            ui->pteMessage->clear();
            ui->pteMessage->setFocus();
        }
        else
        {
            if (ui->lwClients->selectedItems().isEmpty())
            {
                QMessageBox::warning(this, Constants::programName, "Cannot send a message.\nNo users are selected.");
                return;
            }
            QString selectedClients;
            foreach (QListWidgetItem *item, ui->lwClients->selectedItems())
                selectedClients += item->text() + ",";
            selectedClients.remove(selectedClients.length()-1, 1);
            client->sendMessageToSelected(textFromMessageField, selectedClients);
            ui->pteMessage->clear();
            ui->pteMessage->setFocus();
        }
    }
}

void MainWindow::on_pbSend_clicked()
{
    this->tryToSendMessage();
}

void MainWindow::setDefaults()
{
    this->setWindowTitle(Constants::programName + " - [Not connected]");
    trayIcon->setToolTip(Constants::programName + " (Not connected)");
    this->setFixedSize(784, 623);
    ui->pbDisconnect->setDisabled(true);
    ui->pbConnect->setEnabled(true);
    ui->pbSend->setDisabled(true);
    ui->cbToAll->setDisabled(true);
    ui->pteMessage->setFocusPolicy(Qt::StrongFocus);
    ui->pteMessage->setDisabled(true);
    ui->lwClients->setDisabled(true);
    ui->pbConnect->setFocus();
    ui->leName->setReadOnly(true);
    ui->pbSignInOut->setDisabled(true);

    ui->leName->setText(this->loadOneSetting("usernameValue", "").toString());
    ui->leHost->setText(this->loadOneSetting("hostValue", "127.0.0.1").toString());
    ui->sbPort->setValue(this->loadOneSetting("portValue", 1616).toInt());
    ui->cbAutoSignIn->setChecked(this->loadOneSetting("autoSignInState", false).toBool());

    ui->teLogArea->append("<span style=\"color:#ffffff;\">-- TO APPEND --</span>&nbsp;");
}

void MainWindow::on_cbAutoSignIn_toggled(bool checked)
{
    this->saveOneSetting("autoSignInState", checked);
}

void MainWindow::on_pbSignInOut_clicked()
{
    if (ui->pbSignInOut->isChecked())
    {
        this->signIn();
        this->saveOneSetting("usernameValue", ui->leName->text());
    }
    else
    {
        ui->leName->setReadOnly(false);
        ui->pbSignInOut->setText("Sign in");
        ui->pbSignInOut->setChecked(false);

        client->sendCommand(Constants::comDeregisterRequest);
        onAdjustGUIOnDeregister();
        onClearMessageArea();
    }
}


