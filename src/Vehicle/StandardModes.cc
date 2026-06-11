/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

#include "StandardModes.h"
#include "Vehicle.h"
#include "QGCLoggingCategory.h"

QGC_LOGGING_CATEGORY(StandardModesLog, "StandardModesLog")

static QString localizedModeName(const QString& modeName)
{
    const QString upperModeName = modeName.toUpper();

    if (upperModeName == QStringLiteral("STABILIZE"))   return QObject::tr("Stabilize");
    if (upperModeName == QStringLiteral("ACRO"))        return QObject::tr("Acro");
    if (upperModeName == QStringLiteral("ALT_HOLD"))    return QObject::tr("Altitude Hold");
    if (upperModeName == QStringLiteral("AUTO"))        return QObject::tr("Auto");
    if (upperModeName == QStringLiteral("GUIDED"))      return QObject::tr("Guided");
    if (upperModeName == QStringLiteral("LOITER"))      return QObject::tr("Loiter");
    if (upperModeName == QStringLiteral("RTL"))         return QObject::tr("RTL");
    if (upperModeName == QStringLiteral("CIRCLE"))      return QObject::tr("Circle");
    if (upperModeName == QStringLiteral("LAND"))        return QObject::tr("Land");
    if (upperModeName == QStringLiteral("DRIFT"))       return QObject::tr("Drift");
    if (upperModeName == QStringLiteral("SPORT"))       return QObject::tr("Sport");
    if (upperModeName == QStringLiteral("FLIP"))        return QObject::tr("Flip");
    if (upperModeName == QStringLiteral("AUTOTUNE"))    return QObject::tr("Autotune");
    if (upperModeName == QStringLiteral("POSHOLD"))     return QObject::tr("Position Hold");
    if (upperModeName == QStringLiteral("BRAKE"))       return QObject::tr("Brake");
    if (upperModeName == QStringLiteral("THROW"))       return QObject::tr("Throw");
    if (upperModeName == QStringLiteral("AVOID_ADSB"))  return QObject::tr("Avoid ADSB");
    if (upperModeName == QStringLiteral("GUIDED_NOGPS"))return QObject::tr("Guided No GPS");
    if (upperModeName == QStringLiteral("SMARTRTL"))    return QObject::tr("Smart RTL");
    if (upperModeName == QStringLiteral("FLOWHOLD"))    return QObject::tr("Flow Hold");
    if (upperModeName == QStringLiteral("FOLLOW"))      return QObject::tr("Follow");
    if (upperModeName == QStringLiteral("ZIGZAG"))      return QObject::tr("ZigZag");
    if (upperModeName == QStringLiteral("SYSTEMID"))    return QObject::tr("SystemID");
    if (upperModeName == QStringLiteral("AUTOROTATE"))  return QObject::tr("AutoRotate");
    if (upperModeName == QStringLiteral("AUTO_RTL"))    return QObject::tr("AutoRTL");
    if (upperModeName == QStringLiteral("TURTLE"))      return QObject::tr("Turtle");

    return modeName;
}

static void requestMessageResultHandler(void *resultHandlerData, MAV_RESULT result,
                                        [[maybe_unused]] Vehicle::RequestMessageResultHandlerFailureCode_t failureCode,
                                        const mavlink_message_t &message)
{
    StandardModes* standardModes = static_cast<StandardModes*>(resultHandlerData);
    standardModes->gotMessage(result, message);
}

StandardModes::StandardModes(QObject *parent, Vehicle *vehicle)
        : QObject(parent), _vehicle(vehicle)
{
}

void StandardModes::gotMessage(MAV_RESULT result, const mavlink_message_t &message)
{
    _requestActive = false;
    if (_wantReset) {
        _wantReset = false;
        request();
        return;
    }

    if (result == MAV_RESULT_ACCEPTED) {
        mavlink_available_modes_t availableModes;
        mavlink_msg_available_modes_decode(&message, &availableModes);
        bool cannotBeSet = availableModes.properties & MAV_MODE_PROPERTY_NOT_USER_SELECTABLE;
        bool advanced = availableModes.properties & MAV_MODE_PROPERTY_ADVANCED;
        availableModes.mode_name[sizeof(availableModes.mode_name)-1] = '\0';
        QString name = localizedModeName(QString::fromUtf8(availableModes.mode_name));
        switch (availableModes.standard_mode) {
            case MAV_STANDARD_MODE_POSITION_HOLD:
                name = QObject::tr("Position");
                break;
            case MAV_STANDARD_MODE_ORBIT:
                name = QObject::tr("Orbit");
                cannotBeSet = true; // These are exposed in the UI as separate buttons
                break;
            case MAV_STANDARD_MODE_CRUISE:
                name = QObject::tr("Cruise");
                break;
            case MAV_STANDARD_MODE_ALTITUDE_HOLD:
                name = QObject::tr("Altitude");
                break;
            case MAV_STANDARD_MODE_SAFE_RECOVERY:
                name = QObject::tr("Safe Recovery");
                cannotBeSet = true; // These are exposed in the UI as separate buttons
                break;
            case MAV_STANDARD_MODE_MISSION:
                name = QObject::tr("Mission");
                break;
            case MAV_STANDARD_MODE_LAND:
                name = QObject::tr("Land");
                cannotBeSet = true; // These are exposed in the UI as separate buttons
                break;
            case MAV_STANDARD_MODE_TAKEOFF:
                name = QObject::tr("Takeoff");
                cannotBeSet = true; // These are exposed in the UI as separate buttons
                break;
        }

        qCDebug(StandardModesLog) << "Available mode received - name:" << name <<
            "index:" << availableModes.mode_index <<
            "standard_mode:" << availableModes.standard_mode <<
            "advanced:" << advanced <<
            "cannotBeSet:" << cannotBeSet <<
            "custom_mode:" << availableModes.custom_mode;

        _modeList += FirmwareFlightMode{
            name,
            availableModes.standard_mode,
            availableModes.custom_mode,
            !cannotBeSet,
            advanced,
            true,  // fixed wing - Since we don't know at this point we assume fixed wing support
            true   // multi-rotor - Since we don't know at this point we assume multi-rotor support as well
        };

        if (availableModes.mode_index >= availableModes.number_modes) { // We are done
            qCDebug(StandardModesLog) << "Completed, num modes:" << availableModes.number_modes;
            ensureUniqueModeNames();
            _vehicle->firmwarePlugin()->updateAvailableFlightModes(_modeList);
            emit modesUpdated();
            emit requestCompleted();
        } else {
            requestMode(availableModes.mode_index + 1);
        }
    } else {
        qCDebug(StandardModesLog) << "Failed to retrieve available modes - REQUEST_MESSAGE:MAV_RESULT" << result;
        emit requestCompleted();
    }
}

void StandardModes::ensureUniqueModeNames()
{
    // Ensure mode names are unique. This should generally already be the case, but e.g. during development when
    // restarting dynamic modes, it might not be.
    for (auto iter = _modeList.begin(); iter != _modeList.end(); ++iter) {
        int duplicateIdx = 0;
        for (auto iter2 = std::next(iter); iter2 != _modeList.end(); ++iter2) {
            if ((*iter).mode_name == (*iter2).mode_name) {
                (*iter2).mode_name += QStringLiteral(" (%1)").arg(duplicateIdx + 1);
                ++duplicateIdx;
            }
        }
    }
}

void StandardModes::request()
{
    if (_requestActive) {
        // If we are in the middle of waiting for a request, wait for the response first
        _wantReset = true;
        return;
    }

    qCDebug(StandardModesLog) << "Requesting available modes";
    // Request one at a time. This could be improved by requesting all, but we can't use Vehicle::requestMessage for that
    _modeList.clear();
    StandardModes::requestMode(1);
}

void StandardModes::requestMode(int modeIndex)
{
    _requestActive = true;
    _vehicle->requestMessage(
            requestMessageResultHandler,
            this,
            MAV_COMP_ID_AUTOPILOT1,
            MAVLINK_MSG_ID_AVAILABLE_MODES, modeIndex);
}

void StandardModes::availableModesMonitorReceived(uint8_t seq)
{
    if (_lastSeq != seq) {
        qCDebug(StandardModesLog) << "Available modes changed, re-requesting";
        _lastSeq = seq;
        request();
    }
}
