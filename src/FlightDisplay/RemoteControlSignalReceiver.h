/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#pragma once

#include <QtCore/QObject>
#include <QtCore/QVariantMap>
#include <QtCore/QString>

class QUdpSocket;

class RemoteControlSignalReceiver : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(bool listening READ listening NOTIFY listeningChanged)
    Q_PROPERTY(int port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(QString lastPayload READ lastPayload NOTIFY lastPayloadChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QVariantMap latestValues READ latestValues NOTIFY latestValuesChanged)

public:
    explicit RemoteControlSignalReceiver(QObject *parent = nullptr);
    ~RemoteControlSignalReceiver() override;

    bool active() const { return _active; }
    void setActive(bool active);

    bool listening() const { return _listening; }

    int port() const { return _port; }
    void setPort(int port);

    QString lastPayload() const { return _lastPayload; }
    QString statusText() const { return _statusText; }
    QVariantMap latestValues() const { return _latestValues; }

    Q_INVOKABLE void clear();

signals:
    void activeChanged();
    void listeningChanged();
    void portChanged();
    void lastPayloadChanged();
    void statusTextChanged();
    void latestValuesChanged();

private slots:
    void _readPendingDatagrams();

private:
    void _start();
    void _stop();
    void _setListening(bool listening);
    void _setStatusText(const QString &statusText);
    QString _formatPayload(const QByteArray &payload) const;
    QVariantMap _extractValues(const QJsonDocument &json) const;
    static QString _jsonValueToString(const QJsonValue &value);
    static QString _normalizeDelimitedNumberString(QString value);

    QUdpSocket *_socket = nullptr;
    bool _active = false;
    bool _listening = false;
    quint16 _port = 16789;
    QString _lastPayload;
    QString _statusText;
    QVariantMap _latestValues;
};
