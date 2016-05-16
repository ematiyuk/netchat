#ifndef UTILS_H
#define UTILS_H

#include <QString>

class Utils
{
public:
    Utils();

    QString replaceWebLinksInText(QString text);
    QString shortenWebLinkString(QString webLinkStr);
    QString shortenForMessageInTray(QString text);
};

#endif // UTILS_H
