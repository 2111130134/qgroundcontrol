/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.Palette
import QGroundControl.ScreenTools

Item {
    id: _root

    property bool   windowVisible:          true
    property real   areaRatio:              0.25
    property real   alpha:                  0.75
    property real   defaultAspectRatio:     16 / 9
    property alias  receiver:               remoteControlSignalReceiver
    readonly property var _values:          remoteControlSignalReceiver.latestValues

    visible:    windowVisible
    width:      Math.min(parent ? parent.width * 0.85 : 0, Math.sqrt((parent ? parent.width * parent.height : 0) * areaRatio * defaultAspectRatio))
    height:     width / defaultAspectRatio
    z:          QGroundControl.zOrderTopMost - 1

    property real _margin:      ScreenTools.defaultFontPixelWidth
    property real _headerH:     ScreenTools.defaultFontPixelHeight * 2.4
    property real _minX:        0
    property real _minY:        0
    property real _maxX:        parent ? Math.max(0, parent.width - width) : 0
    property real _maxY:        parent ? Math.max(0, parent.height - height) : 0

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    RemoteControlSignalReceiver {
        id:         remoteControlSignalReceiver
        port:       16789
        active:     QGroundControl.settingsManager.flyViewSettings.showRemoteControlSignal.rawValue
    }

    function showWindow() {
        windowVisible = true
        _clampPosition()
    }

    function closeWindow() {
        windowVisible = false
    }

    function _displayValue(key, suffix) {
        const value = _values && _values[key] !== undefined ? _values[key] : ""
        return value === "" ? "--" + suffix : value + suffix
    }

    function _clampPosition() {
        x = Math.max(_minX, Math.min(x, _maxX))
        y = Math.max(_minY, Math.min(y, _maxY))
    }

    Component.onCompleted: {
        x = parent ? Math.max(0, (parent.width - width) / 2) : 0
        y = parent ? Math.max(0, (parent.height - height) / 2) : 0
    }

    onWidthChanged:    _clampPosition()
    onHeightChanged:   _clampPosition()

    Connections {
        target: parent

        function onWidthChanged() {
            _root._clampPosition()
        }

        function onHeightChanged() {
            _root._clampPosition()
        }
    }

    Rectangle {
        anchors.fill:   parent
        radius:         ScreenTools.defaultFontPixelWidth * 0.6
        color:          qgcPal.window
        opacity:        _root.alpha
        border.color:   qgcPal.text
        border.width:   1
    }

    MouseArea {
        anchors.fill:       parent
        drag.target:        _root
        drag.axis:          Drag.XAndYAxis
        drag.minimumX:      _root._minX
        drag.maximumX:      _root._maxX
        drag.minimumY:      _root._minY
        drag.maximumY:      _root._maxY
        preventStealing:    true
    }

    ColumnLayout {
        anchors.fill:       parent
        anchors.margins:    _margin
        spacing:            ScreenTools.defaultFontPixelHeight * 0.35

        RowLayout {
            Layout.fillWidth:       true
            Layout.preferredHeight: _headerH
            spacing:                _margin

            QGCLabel {
                Layout.fillWidth:   true
                text:               qsTr("Remote Control Signal")
                font.bold:          true
                color:              qgcPal.text
                elide:              Text.ElideRight
            }

            QGCButton {
                Layout.preferredWidth:     _headerH
                Layout.preferredHeight:    _headerH
                text:                       "X"
                heightFactor:               0
                onClicked:                  _root.closeWindow()
            }
        }

        GridLayout {
            Layout.fillWidth:   true
            Layout.fillHeight:  true
            columns:            4
            columnSpacing:      _margin * 0.8
            rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.4

            QGCLabel {
                Layout.fillWidth:   true
                Layout.columnSpan:  4
                text:               remoteControlSignalReceiver.statusText
                color:              qgcPal.text
                elide:              Text.ElideRight
            }

            QGCLabel { Layout.fillWidth: true; text: qsTr("airRSSI1"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("airRSSI1", " dBm") }
            QGCLabel { Layout.fillWidth: true; text: qsTr("gndRSSI1"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("gndRSSI1", " dBm") }

            QGCLabel { Layout.fillWidth: true; text: qsTr("airRSSI2"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("airRSSI2", " dBm") }
            QGCLabel { Layout.fillWidth: true; text: qsTr("gndRSSI2"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("gndRSSI2", " dBm") }

            QGCLabel { Layout.fillWidth: true; text: qsTr("airSNR"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("airSNR", " dB") }
            QGCLabel { Layout.fillWidth: true; text: qsTr("gndSNR"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("gndSNR", " dB") }

            QGCLabel { Layout.fillWidth: true; text: qsTr("airPass"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("airPass", "") }
            QGCLabel { Layout.fillWidth: true; text: qsTr("gndPass"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("gndPass", "") }

            QGCLabel { Layout.fillWidth: true; text: qsTr("airFailed"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("airFailed", "") }
            QGCLabel { Layout.fillWidth: true; text: qsTr("gndFailed"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("gndFailed", "") }

            QGCLabel { Layout.fillWidth: true; text: qsTr("airAnt"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("airAnt", "") }
            QGCLabel { Layout.fillWidth: true; text: qsTr("gndAnt"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("gndAnt", "") }

            QGCLabel { Layout.fillWidth: true; text: qsTr("freq"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("freq", "") }
            QGCLabel { Layout.fillWidth: true; text: qsTr("mcs"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("mcs", "") }

            QGCLabel { Layout.fillWidth: true; text: qsTr("range"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("range", " m") }
            QGCLabel { Layout.fillWidth: true; text: qsTr("rate"); horizontalAlignment: Text.AlignRight }
            QGCLabel { Layout.fillWidth: true; text: _displayValue("rate", " kbps") }
        }
    }
}
