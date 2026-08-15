import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: textField
    anchors.fill: parent
    horizontalAlignment: TextInput.AlignHCenter
    color: Theme.textPrimary
    font.family: Theme.fontFamily
    font.pixelSize: Theme.fontSizeBase

    background: Rectangle {
        color: Theme.bgControl
        border.color: Theme.accent
    }
}
