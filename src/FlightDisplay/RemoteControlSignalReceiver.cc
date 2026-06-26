/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "RemoteControlSignalReceiver.h"

#include <QtCore/QJsonDocument>
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

void RemoteControlSignalReceiver::setPort(quint16 port)
{
    if (_port == port) {
        return;
    }

    _port = port;
    emit portChanged();

    if (_active) {
        _stop();
        _start();
    }
}

void RemoteControlSignalReceiver::clear()
{
    if (_lastPayload.isEmpty()) {
        return;
    }

    _lastPayload.clear();
    emit lastPayloadChanged();
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
        _lastPayload = _formatPayload(datagram.data());
        emit lastPayloadChanged();
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
