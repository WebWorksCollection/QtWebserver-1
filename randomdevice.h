#pragma once

#include <QIODevice>

class RandomDevice : public QIODevice
{
    Q_OBJECT

public:
    explicit RandomDevice(qint64 size, QObject *parent = Q_NULLPTR);

    // QIODevice interface
    qint64 pos() const Q_DECL_OVERRIDE;
    qint64 size() const Q_DECL_OVERRIDE;

protected:
    // QIODevice interface
    qint64 readData(char *data, qint64 maxlen) Q_DECL_OVERRIDE;
    qint64 writeData(const char *data, qint64 len) Q_DECL_OVERRIDE;

private:
    qint64 m_pos;
    qint64 m_size;
};
