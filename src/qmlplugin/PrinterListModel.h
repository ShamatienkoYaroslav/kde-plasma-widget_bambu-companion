#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>
#include <qqmlregistration.h>

class PrinterListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        StatusRole,
    };

    explicit PrinterListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    struct PrinterEntry {
        QString name;
        QString status;
    };

    QList<PrinterEntry> m_printers;
};
