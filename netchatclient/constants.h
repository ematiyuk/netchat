#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

namespace Constants {

static const QString constNameUnknown = ".Unknown";
static const quint8 comRegisterRequest = 1;
static const quint8 comRegisteredClients = 2;
static const quint8 comClientJoined = 3;
static const quint8 comClientLeft = 4;
static const quint8 comMessageToAll = 5;
static const quint8 comMessageToClients = 6;
static const quint8 comPublicServerMessage = 7;
static const quint8 comPrivateServerMessage = 8;
static const quint8 comRegistrationSuccess = 9;
static const quint8 comDisconnectClient = 10;
static const quint8 comDeregisterRequest = 11;
static const quint8 comDeregisterClient = 14;
static const quint8 comPing = 15;
static const quint8 comClientConnected = 16;

static const quint8 comErrClientExists = 201;
static const quint8 comErrNameInvalid = 202;
static const quint8 comErrNameUsed = 203;
static const quint8 comErrNameIllegal = 204;

static const QString programName = "NetChatClient";
}

#endif // CONSTANTS_H
