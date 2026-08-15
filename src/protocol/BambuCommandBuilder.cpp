#include "BambuCommandBuilder.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
QByteArray printCommand(const QString &sequenceId, const QString &command)
{
    QJsonObject print;
    print.insert(QStringLiteral("sequence_id"), sequenceId);
    print.insert(QStringLiteral("command"), command);
    print.insert(QStringLiteral("param"), QString());

    QJsonObject root;
    root.insert(QStringLiteral("print"), print);

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}
}

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

QByteArray BambuCommandBuilder::pause(const QString &sequenceId)
{
    return printCommand(sequenceId, QStringLiteral("pause"));
}

QByteArray BambuCommandBuilder::resume(const QString &sequenceId)
{
    return printCommand(sequenceId, QStringLiteral("resume"));
}

QByteArray BambuCommandBuilder::stop(const QString &sequenceId)
{
    return printCommand(sequenceId, QStringLiteral("stop"));
}

QByteArray BambuCommandBuilder::skipObjects(const QString &sequenceId, const QList<int> &objectIds)
{
    QJsonArray objList;
    for (int id : objectIds) {
        objList.append(id);
    }

    QJsonObject print;
    print.insert(QStringLiteral("sequence_id"), sequenceId);
    print.insert(QStringLiteral("command"), QStringLiteral("skip_objects"));
    print.insert(QStringLiteral("timestamp"), QJsonValue(static_cast<double>(QDateTime::currentSecsSinceEpoch())));
    print.insert(QStringLiteral("obj_list"), objList);

    QJsonObject root;
    root.insert(QStringLiteral("print"), print);

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}
