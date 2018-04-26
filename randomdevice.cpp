#include "randomdevice.h"

#include <QDebug>

RandomDevice::RandomDevice(qint64 size, QObject *parent) :
    QIODevice(parent),
    m_pos(0),
    m_size(size)
{
}

qint64 RandomDevice::pos() const
{
    return m_pos;
}

qint64 RandomDevice::size() const
{
    return m_size;
}

qint64 RandomDevice::readData(char *data, qint64 maxlen)
{
    std::generate_n(data, maxlen, std::rand);
    m_pos += maxlen;
    return maxlen;
}

qint64 RandomDevice::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data)
    Q_UNUSED(len)

    qCritical() << "cannot write to this device!";
    return 0;
}
