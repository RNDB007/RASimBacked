// clientsession.h
#pragma once
#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDataStream>

class ClientSession : public QObject
{
    Q_OBJECT
public:
    explicit ClientSession(qintptr socketDescriptor, QObject* parent = nullptr);
    void sendJson(const QJsonObject& json);
    QString getClientId() const;

signals:
    void dataReceived(const QString& id, const QJsonObject& json, QTcpSocket* socket);
    void disconnected(const QString& id);

private slots:
    void onReadyRead();

private:
    QTcpSocket* m_socket;
    QByteArray m_buffer;
    quint32 m_expectedSize = 0;
    QString clientId;
};
