/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.ScreenTools

Item {
    id: _root

    property bool   windowVisible:          true
    property real   areaRatio:              0.25
    property real   alpha:                  0.75
    property real   defaultAspectRatio:     16 / 9

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

    function showWindow() {
        windowVisible = true
        _clampPosition()
    }

    function closeWindow() {
        windowVisible = false
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
            rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.35

            Repeater {
                model: [
                    qsTr("airRSSI1"), "-- dBm", qsTr("gndRSSI1"), "-- dBm",
                    qsTr("airRSSI2"), "-- dBm", qsTr("gndRSSI2"), "-- dBm",
                    qsTr("airSNR"), "-- dB", qsTr("gndSNR"), "-- dB",
                    qsTr("airPass"), "--", qsTr("gndPass"), "--",
                    qsTr("airFailed"), "--", qsTr("gndFailed"), "--",
                    qsTr("airAnt"), "--", qsTr("gndAnt"), "--",
                    qsTr("freq"), "--", qsTr("mcs"), "--",
                    qsTr("range"), "-- m", qsTr("rate"), "-- kbps"
                ]

                QGCLabel {
                    Layout.fillWidth:   true
                    text:               modelData
                    color:              qgcPal.text
                    horizontalAlignment: index % 2 === 0 ? Text.AlignRight : Text.AlignLeft
                    elide:              Text.ElideRight
                }
            }
        }
    }
}
