import QtQuick
import QtQuick.Controls.Basic

Slider {
    id: hSlider
    orientation: Qt.Horizontal
    from: 0.0
    to: 1.0
    stepSize: 0.01
    implicitHeight: 46
    implicitWidth: 100

    signal resetRequested

    background: Rectangle {
        x: hSlider.leftPadding
        y: hSlider.topPadding + hSlider.availableHeight / 2 - height / 2
        width: hSlider.availableWidth
        height: 6
        color: Theme.controlTrack
        border.color: Theme.bgSubPanel
        border.width: 1

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            width: hSlider.position * parent.width
            height: parent.height - 2 * parent.border.width
            color: Theme.accent
        }
    }

    handle: Image {
        x: hSlider.leftPadding + hSlider.visualPosition * hSlider.availableWidth - width / 2
        y: hSlider.topPadding + hSlider.availableHeight / 2 - height / 2
        width: 35
        height: 20
        source: "qrc:/icons/thumb_h.svg"
        sourceSize: Qt.size(width, height)
    }
}
