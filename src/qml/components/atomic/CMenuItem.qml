import QtQuick
import QtQuick.Controls.Basic

MenuItem {
    id: item
    implicitHeight: 26

    contentItem: Text {
        leftPadding: Theme.spacingM
        rightPadding: Theme.spacingM
        text: item.text
        color: item.enabled ? Theme.textPrimary : Theme.textMuted
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeBase
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: 180
        color: item.highlighted ? Theme.accent : "transparent"
        radius: Theme.radiusXS
    }
}
