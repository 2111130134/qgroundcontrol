/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RemoteControlSignalReceiver.h"

#include <QtCore/QtGlobal>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QRegularExpression>
#include <QtCore/QJsonValue>
#include <QtCore/QStringList>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QNetworkDatagram>
#include <QtNetwork/QUdpSocket>

RemoteControlSignalReceiver::RemoteControlSignalReceiver(QObject *parent)
    : QObject(parent)
{
    _setStatusText(tr("Not listening"));
}

RemoteControlSignalReceiver::~RemoteControlSignalReceiver()
{
    _stop();
}

void RemoteControlSignalReceiver::setActive(bool active)
{
    if (_active == active) {
        return;
    }

    _active = active;
    emit activeChanged();

    if (_active) {
        _start();
    } else {
        _stop();
    }
}

void RemoteControlSignalReceiver::setPort(int port)
{
    const quint16 normalizedPort = static_cast<quint16>(qBound(0, port, 65535));
    if (_port == normalizedPort) {
        return;
    }

    _port = normalizedPort;
    emit portChanged();

    if (_active) {
        _stop();
        _start();
    }
}

void RemoteControlSignalReceiver::clear()
{
    if (_lastPayload.isEmpty() && _latestValues.isEmpty()) {
        return;
    }

    _lastPayload.clear();
    _latestValues.clear();
    emit lastPayloadChanged();
    emit latestValuesChanged();
}

void RemoteControlSignalReceiver::_start()
{
    if (_socket) {
        return;
    }

    _socket = new QUdpSocket(this);
    connect(_socket, &QUdpSocket::readyRead, this, &RemoteControlSignalReceiver::_readPendingDatagrams);

    const bool bound = _socket->bind(QHostAddress::AnyIPv4,
                                     _port,
                                     QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);
    if (!bound) {
        _setStatusText(tr("UDP %1 listen failed: %2").arg(_port).arg(_socket->errorString()));
        _socket->deleteLater();
        _socket = nullptr;
        _setListening(false);
        return;
    }

    _setListening(true);
    _setStatusText(tr("Listening on UDP %1").arg(_port));
}

void RemoteControlSignalReceiver::_stop()
{
    if (!_socket) {
        _setListening(false);
        _setStatusText(tr("Not listening"));
        return;
    }

    _socket->close();
    _socket->deleteLater();
    _socket = nullptr;
    _setListening(false);
    _setStatusText(tr("Not listening"));
}

void RemoteControlSignalReceiver::_readPendingDatagrams()
{
    if (!_socket) {
        return;
    }

    while (_socket->hasPendingDatagrams()) {
        const QNetworkDatagram datagram = _socket->receiveDatagram();
        QJsonParseError parseError;
        const QJsonDocument json = QJsonDocument::fromJson(datagram.data(), &parseError);
        if (parseError.error == QJsonParseError::NoError && !json.isNull()) {
            _lastPayload = QString::fromUtf8(json.toJson(QJsonDocument::Indented));
            _latestValues = _extractValues(json);
        } else {
            _lastPayload = QString::fromUtf8(datagram.data());
            _latestValues.clear();
        }
        emit lastPayloadChanged();
        emit latestValuesChanged();
        _setStatusText(tr("Received %1 bytes from %2:%3")
                           .arg(datagram.data().size())
                           .arg(datagram.senderAddress().toString())
                           .arg(datagram.senderPort()));
    }
}

void RemoteControlSignalReceiver::_setListening(bool listening)
{
    if (_listening == listening) {
        return;
    }

    _listening = listening;
    emit listeningChanged();
}

void RemoteControlSignalReceiver::_setStatusText(const QString &statusText)
{
    if (_statusText == statusText) {
        return;
    }

    _statusText = statusText;
    emit statusTextChanged();
}

QString RemoteControlSignalReceiver::_formatPayload(const QByteArray &payload) const
{
    QJsonParseError parseError;
    const QJsonDocument json = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error == QJsonParseError::NoError && !json.isNull()) {
        return QString::fromUtf8(json.toJson(QJsonDocument::Indented));
    }

    return QString::fromUtf8(payload);
}

QVariantMap RemoteControlSignalReceiver::_extractValues(const QJsonDocument &json) const
{
    QVariantMap values;
    if (!json.isObject()) {
        return values;
    }

    const QJsonObject obj = json.object();
    values.insert(QStringLiteral("airRSSI1"), _jsonValueToString(obj.value(QStringLiteral("rssi1_a"))));
    values.insert(QStringLiteral("gndRSSI1"), _jsonValueToString(obj.value(QStringLiteral("rssi1_g"))));
    values.insert(QStringLiteral("airRSSI2"), _jsonValueToString(obj.value(QStringLiteral("rssi2_a"))));
    values.insert(QStringLiteral("gndRSSI2"), _jsonValueToString(obj.value(QStringLiteral("rssi2_g"))));
    values.insert(QStringLiteral("airSNR"), _normalizeDelimitedNumberString(_jsonValueToString(obj.value(QStringLiteral("snr_a")))));
    values.insert(QStringLiteral("gndSNR"), _normalizeDelimitedNumberString(_jsonValueToString(obj.value(QStringLiteral("snr_g")))));
    values.insert(QStringLiteral("airPass"), _jsonValueToString(obj.value(QStringLiteral("pass_a"))));
    values.insert(QStringLiteral("gndPass"), _jsonValueToString(obj.value(QStringLiteral("pass_g"))));
    values.insert(QStringLiteral("airFailed"), _jsonValueToString(obj.value(QStringLiteral("failed_a"))));
    values.insert(QStringLiteral("gndFailed"), _jsonValueToString(obj.value(QStringLiteral("failed_g"))));
    values.insert(QStringLiteral("airAnt"), _jsonValueToString(obj.value(QStringLiteral("ant_a"))));
    values.insert(QStringLiteral("gndAnt"), _jsonValueToString(obj.value(QStringLiteral("ant_g"))));
    values.insert(QStringLiteral("freq"), _jsonValueToString(obj.value(QStringLiteral("freq_tx"))));
    values.insert(QStringLiteral("mcs"), _jsonValueToString(obj.value(QStringLiteral("mcs"))));
    values.insert(QStringLiteral("range"), _jsonValueToString(obj.value(QStringLiteral("distance"))));
    values.insert(QStringLiteral("rate"), QString());

    return values;
}

QString RemoteControlSignalReceiver::_jsonValueToString(const QJsonValue &value)
{
    if (value.isString()) {
        return value.toString();
    }
    if (value.isDouble()) {
        return QString::number(value.toDouble());
    }
    if (value.isBool()) {
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    }
    return QString();
}

QString RemoteControlSignalReceiver::_normalizeDelimitedNumberString(QString value)
{
    value.replace(QLatin1Char('"'), QLatin1Char(' '));
    value.replace(QLatin1Char('/'), QLatin1Char(' '));
    const QStringList parts = value.split(QRegularExpression(QStringLiteral("[\\s,]+")), Qt::SkipEmptyParts);
    return parts.isEmpty() ? QString() : parts.constFirst();
}
