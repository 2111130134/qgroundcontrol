/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QtLocation
import QtPositioning
import QtQuick.Window
import QtQml.Models

import QGroundControl
import QGroundControl.Controllers
import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.FlightDisplay
import QGroundControl.FlightMap
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Vehicle

// 3D Viewer modules
import Viewer3D

Item {
    id: _root

    // These should only be used by MainRootWindow
    property var planController:    _planController
    property var guidedController:  _guidedController

    // Properties of UTM adapter
    property bool utmspSendActTrigger: false

    PlanMasterController {
        id:                     _planController
        flyView:                true
        Component.onCompleted:  start()
    }

    property bool   _mainWindowIsMap:       mapControl.pipState.state === mapControl.pipState.fullState
    property bool   _isFullWindowItemDark:  _mainWindowIsMap ? mapControl.isSatelliteMap : true
    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property var    _missionController:     _planController.missionController
    property var    _geoFenceController:    _planController.geoFenceController
    property var    _rallyPointController:  _planController.rallyPointController
    property real   _margins:               ScreenTools.defaultFontPixelWidth / 2
    property var    _guidedController:      guidedActionsController
    property var    _guidedValueSlider:     guidedValueSlider
    property var    _widgetLayer:           widgetLayer
    property real   _toolsMargin:           ScreenTools.defaultFontPixelWidth * 0.75
    property rect   _centerViewport:        Qt.rect(0, 0, width, height)
    property real   _rightPanelWidth:       ScreenTools.defaultFontPixelWidth * 30
    property var    _mapControl:            mapControl

    property real   _fullItemZorder:    0
    property real   _pipItemZorder:     QGroundControl.zOrderWidgets

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    function _calcCenterViewPort() {
        var newToolInset = Qt.rect(0, 0, width, height)
        toolstrip.adjustToolInset(newToolInset)
    }

    function dropMainStatusIndicatorTool() {
        toolbar.dropMainStatusIndicatorTool();
    }

    QGCToolInsets {
        id:                     _toolInsets
        leftEdgeBottomInset:    _pipView.leftEdgeBottomInset
        bottomEdgeLeftInset:    _pipView.bottomEdgeLeftInset
    }

    FlyViewToolBar {
        id:         toolbar
        visible:    !QGroundControl.videoManager.fullScreen
    }

    Item {
        id:                 mapHolder
        anchors.top:        toolbar.bottom
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left
        anchors.right:      parent.right

        FlyViewMap {
            id:                     mapControl
            planMasterController:   _planController
            rightPanelWidth:        ScreenTools.defaultFontPixelHeight * 9
            pipView:                _pipView
            pipMode:                !_mainWindowIsMap
            toolInsets:             customOverlay.totalToolInsets
            mapName:                "FlightDisplayView"
            enabled:                !viewer3DWindow.isOpen
        }

        FlyViewVideo {
            id:         videoControl
            pipView:    _pipView
        }

        PipView {
            id:                     _pipView
            anchors.left:           parent.left
            anchors.bottom:         parent.bottom
            anchors.margins:        _toolsMargin
            item1IsFullSettingsKey: "MainFlyWindowIsMap"
            item1:                  mapControl
            item2:                  QGroundControl.videoManager.hasVideo ? videoControl : null
            show:                   QGroundControl.videoManager.hasVideo && !QGroundControl.videoManager.fullScreen &&
                                        (videoControl.pipState.state === videoControl.pipState.pipState || mapControl.pipState.state === mapControl.pipState.pipState)
            z:                      QGroundControl.zOrderWidgets

            property real leftEdgeBottomInset: visible ? width + anchors.margins : 0
            property real bottomEdgeLeftInset: visible ? height + anchors.margins : 0
        }

        FlyViewWidgetLayer {
            id:                     widgetLayer
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            anchors.left:           parent.left
            anchors.right:          guidedValueSlider.visible ? guidedValueSlider.left : parent.right
            z:                      _fullItemZorder + 2 // we need to add one extra layer for map 3d viewer (normally was 1)
            parentToolInsets:       _toolInsets
            mapControl:             _mapControl
            visible:                !QGroundControl.videoManager.fullScreen
            utmspActTrigger:        utmspSendActTrigger
            isViewer3DOpen:         viewer3DWindow.isOpen
        }

        FlyViewCustomLayer {
            id:                 customOverlay
            anchors.fill:       widgetLayer
            z:                  _fullItemZorder + 2
            parentToolInsets:   widgetLayer.totalToolInsets
            mapControl:         _mapControl
            visible:            !QGroundControl.videoManager.fullScreen
        }

        RemoteControlSignalWindow {
            id:                 remoteControlSignalWindow
            alpha:              0.75
            areaRatio:          0.25
            visible:            QGroundControl.settingsManager.flyViewSettings.showRemoteControlSignal.rawValue &&
                                    windowVisible &&
                                    !QGroundControl.videoManager.fullScreen

            Connections {
                target: QGroundControl.settingsManager.flyViewSettings.showRemoteControlSignal

                function onRawValueChanged() {
                    if (QGroundControl.settingsManager.flyViewSettings.showRemoteControlSignal.rawValue) {
                        remoteControlSignalWindow.showWindow()
                    }
                }
            }
        }

        Rectangle {
            id:                 remoteControlSignalShowButton
            width:              ScreenTools.defaultFontPixelHeight * 3
            height:             width
            radius:             ScreenTools.defaultFontPixelHeight * 0.5  
            z:                  QGroundControl.zOrderTopMost + 2          //优先级最高
            color:              qgcPal.window
            opacity:            0.75
            border.color:       qgcPal.text
            border.width:       1
            visible:            QGroundControl.settingsManager.flyViewSettings.showRemoteControlSignal.rawValue &&
                                    !remoteControlSignalWindow.windowVisible &&
                                    !QGroundControl.videoManager.fullScreen

            // 新增：初始居中并支持拖拽后位置约束
            Component.onCompleted: {
                x = parent ? (parent.width - width) / 2 : 0
                y = parent ? (parent.height - height) / 2 : 0
            }

            property real _btnMinX: 0
            property real _btnMinY: 0
            property real _btnMaxX: parent ? Math.max(0, parent.width - width) : 0
            property real _btnMaxY: parent ? Math.max(0, parent.height - height) : 0

            function _clampBtnPosition() {
                x = Math.max(_btnMinX, Math.min(x, _btnMaxX))
                y = Math.max(_btnMinY, Math.min(y, _btnMaxY))
            }

            onWidthChanged:  _clampBtnPosition()
            onHeightChanged: _clampBtnPosition()

            Connections {
                target: mapHolder
                function onWidthChanged()  { _clampBtnPosition() }
                function onHeightChanged() { _clampBtnPosition() }
            }

            QGCColoredImage {
                anchors.centerIn:   parent
                width:              parent.width * 0.58
                height:             width
                source:             "qrc:/InstrumentValueIcons/view-show.svg"
                color:              qgcPal.text
                fillMode:           Image.PreserveAspectFit
            }

            MouseArea {
                anchors.fill:   parent
                drag.target:    remoteControlSignalShowButton
                drag.axis:      Drag.XAndYAxis
                drag.minimumX:  0
                drag.maximumX:  mapHolder.width - remoteControlSignalShowButton.width
                drag.minimumY:  0
                drag.maximumY:  mapHolder.height - remoteControlSignalShowButton.height
                onClicked:      drag.active ? null : remoteControlSignalWindow.showWindow()
            }
        }

        // Development tool for visualizing the insets for a paticular layer, show if needed
        FlyViewInsetViewer {
            id:                     widgetLayerInsetViewer
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            anchors.left:           parent.left
            anchors.right:          guidedValueSlider.visible ? guidedValueSlider.left : parent.right
            z:                      widgetLayer.z + 1
            insetsToView:           widgetLayer.totalToolInsets
            visible:                false
        }

        GuidedActionsController {
            id:                 guidedActionsController
            missionController:  _missionController
            guidedValueSlider:     _guidedValueSlider
        }

        //-- Guided value slider (e.g. altitude)
        GuidedValueSlider {
            id:                 guidedValueSlider
            anchors.right:      parent.right
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            z:                  QGroundControl.zOrderTopMost
            visible:            false
        }

        Viewer3D{
            id:                     viewer3DWindow
            anchors.fill:           parent
        }
    }
}
