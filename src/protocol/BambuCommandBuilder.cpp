#include "BambuCommandBuilder.h"

#include <QJsonDocument>
#include <QJsonObject>

QByteArray BambuCommandBuilder::pushAll()
{
    QJsonObject pushing;
    pushing.insert(QStringLiteral("sequence_id"), QStringLiteral("0"));
    pushing.insert(QStringLiteral("command"), QStringLiteral("pushall"));
    pushing.insert(QStringLiteral("version"), 1);
    pushing.insert(QStringLiteral("push_target"), 1);

    QJsonObject root;
    root.insert(QStringLiteral("pushing"), pushing);

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}
