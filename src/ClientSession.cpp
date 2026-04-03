#include "ClientSession.h"

ClientSession::ClientSession(qintptr socketDescriptor, QObject* parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    if (!m_socket->setSocketDescriptor(socketDescriptor)) {
        qCritical() << "Failed to set socket descriptor:" << m_socket->errorString();
        emit disconnected(getClientId());
        return;
    }
    clientId = getClientId();
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, [this]() { emit disconnected(clientId); });
}

QString ClientSession::getClientId() const
{
    return QString::number(m_socket->socketDescriptor());
}

void ClientSession::onReadyRead()
{
    m_buffer.append(m_socket->readAll());

    while (true) {
        if (m_expectedSize == 0) {
            if (m_buffer.size() < (qint64)sizeof(quint32)) {
                return;
            }
            QDataStream stream(m_buffer);
            stream.setByteOrder(QDataStream::LittleEndian);
            quint32 tempSize = 0;
            stream >> tempSize;
            m_expectedSize = tempSize;
            m_buffer.remove(0, sizeof(quint32));
        }

        if (m_buffer.size() < (qint64)m_expectedSize) {
            return;
        }

        QByteArray packetData = m_buffer.left(m_expectedSize);
        m_buffer.remove(0, m_expectedSize);
        m_expectedSize = 0;

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(packetData, &parseError);

        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            emit dataReceived(getClientId(), doc.object(), m_socket);
        }
        else {
            qDebug() << "JSON Parse Error:" << parseError.errorString();
        }
    }
}

void ClientSession::sendJson(const QJsonObject& json)
{
    QByteArray jsonData = QJsonDocument(json).toJson(QJsonDocument::Compact);

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out.setByteOrder(QDataStream::LittleEndian);
    out << (quint32)jsonData.size();
    block.append(jsonData);

    m_socket->write(block);
    m_socket->flush();
}
