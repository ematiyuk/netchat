#include "utils.h"

#include <QRegExp>
#include <QList>

Utils::Utils()
{
}

struct WebLinkStruct
{
    int linkStartPos;
    int linkLength;
    QString linkStr;
};

QString Utils::replaceWebLinksInText(QString text)
{
    QRegExp urlRegExp("(http|https):\\/\\/[^\\s]+");
    urlRegExp.setCaseSensitivity(Qt::CaseInsensitive);
    int count = 0;
    int pos = 0;
    QString linkStr;
    int linkStartPos = 0;
    int linkEndPos = 0;
    QList<WebLinkStruct> linksList;
    while ((pos = urlRegExp.indexIn(text, pos)) != -1) {
        ++count;
        pos += urlRegExp.matchedLength();

        linkStartPos = urlRegExp.pos(0);
        linkEndPos = pos;
        linkStr = text.mid(linkStartPos, (linkEndPos - linkStartPos));
        WebLinkStruct link;
        link.linkStartPos = linkStartPos;
        link.linkLength = linkEndPos - linkStartPos;
        link.linkStr = linkStr;
        linksList.append(link);
    }
    int deltaValue = 0;
    QString templateStr;
    int templateStrLength = 0;
    for (int i = 0; i < linksList.length(); ++i)
    {
        templateStr = "{link#" + QString::number(i+1) + "}";
        if (i == 0)
        {
            text = text.replace(linksList[i].linkStartPos, linksList[i].linkLength, templateStr);
            templateStrLength = templateStr.length();
            deltaValue = linksList[i].linkLength - templateStrLength;
        }
        else
        {
            text = text.replace(linksList[i].linkStartPos - deltaValue, linksList[i].linkLength, templateStr);
            templateStrLength = templateStr.length();
            deltaValue += linksList[i].linkLength - templateStrLength;
        }
    }
    for (int i = 0; i < linksList.length(); ++i)
    {
        templateStr = "{link#" + QString::number(i+1) + "}";
        QString fullWebLink = "<a href=\"" + linksList[i].linkStr + "\" title=\"" + linksList[i].linkStr + "\">" +
                shortenWebLinkString(linksList[i].linkStr) + "</a>";
        text = text.replace(templateStr, fullWebLink);
    }
    return text;
}

QString Utils::shortenWebLinkString(QString webLinkStr)
{
    int webLinkStrLength = webLinkStr.length();
    int lengthLimit = 50;
    if (webLinkStrLength > lengthLimit)
        return webLinkStr.left(lengthLimit) + "..";
    else
        return webLinkStr;
}

QString Utils::shortenForMessageInTray(QString text)
{
    int textLength = text.length();
    int msgLimit = 30;
    if (textLength < msgLimit && text.contains('\n'))
        return "\"" + text.left(text.indexOf('\n')) + " ";
    else if (textLength > msgLimit)
        return "\"" + text.left(msgLimit) + "... \"";
    else
        return "\"" + text + "\"";
}
