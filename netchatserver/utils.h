#ifndef UTILS_H
#define UTILS_H

#include <QString>

class Utils
{
public:
    Utils();

    QString replaceWebLinksInText(QString text);
    QString shortenWebLinkString(QString webLinkStr);
    bool isNameValid(QString name) const;
};

#endif // UTILS_H
