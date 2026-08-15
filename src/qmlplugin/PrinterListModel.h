#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QUuid>
#include <QVariantMap>
#include <qqmlregistration.h>

class PrinterListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    enum Role {
        IdRole = Qt::UserRole + 1,
        NameRole,
        StateRole,
        ProgressRole,
        NozzleTempRole,
        BedTempRole,
        ConnectionStateRole,
    };

    explicit PrinterListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Full status for a single printer, for PrinterDetailView.qml (a ListView
    // delegate only sees the roles above; this covers the rest).
    Q_INVOKABLE QVariantMap printerDetails(const QString &printerId) const;

private:
    void reload();
    int rowForId(const QUuid &id) const;

    QList<QUuid> m_order;
};
