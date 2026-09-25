import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Button {
    id: button

    property string symbol: ""
    property string iconName: ""
    property bool danger: false
    property bool square: false
    property bool big: false
    property bool fontSmall: false
    property bool fullWidth: false
    property bool blinking: false
    property color textColor: Theme.textPrimary
    property color bgColor: "transparent"
    property int iconSize: button.big ? 28 : (button.fontSmall ? 14 : 16)
    property color _bgColor: button.bgColor.a > 0 ? button.bgColor : button.highlighted || button.checked ? Theme.accent : button.danger ? ((button.hovered || button.down) && button.enabled ? Theme.bgDangerHover : Theme.bgDanger) : ((button.hovered || button.down) && button.enabled ? Theme.bgHover : Theme.bgControl)

    readonly property int squareSize: 35
    readonly property int squareSizeBig: 64

    Layout.alignment: Qt.AlignVCenter
    Layout.fillWidth: fullWidth
    opacity: button.enabled ? 1 : 0.3

    contentItem: Item {
        implicitWidth: buttonLabel.implicitWidth
        implicitHeight: buttonLabel.implicitHeight

        Text {
            id: buttonLabel
            anchors.fill: parent
            visible: button.iconName === ""
            text: button.symbol
            color: button.textColor
            font.family: Theme.fontFamily
            font.pixelSize: (fontSmall) ? Theme.fontSizeSmall : Theme.fontSizeBase
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Image {
            id: iconImage
            anchors.centerIn: parent
            visible: button.iconName !== ""
            source: button.iconName !== "" ? "qrc:/icons/" + button.iconName + ".svg" : ""
            sourceSize.width: button.iconSize
            sourceSize.height: button.iconSize
            width: button.iconSize
            height: button.iconSize
            fillMode: Image.PreserveAspectFit
            smooth: true
            mipmap: true
        }
    }

    background: Rectangle {
        id: background
        implicitWidth: button.square ? (button.big ? button.squareSizeBig : button.squareSize) : 35
        implicitHeight: button.square ? (button.big ? button.squareSizeBig : button.squareSize) : 25
        color: button._bgColor
        border.color: button.activeFocus ? Theme.accent : Theme.borderSubtle
        border.width: 1
        radius: Theme.radiusXS
    }

    SequentialAnimation {
        running: button.blinking
        loops: Animation.Infinite
        ColorAnimation {
            target: button
            property: "_bgColor"
            from: background.color
            to: Theme.bgControl
            duration: 500
        }
        ColorAnimation {
            target: button
            property: "_bgColor"
            from: Theme.bgControl
            to: background.color
            duration: 500
        }
    }
}
