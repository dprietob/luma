import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: indicator
    width: 8
    height: 8
    radius: width / 2
    color: hasTrack ? Theme.trackLoaded : Theme.trackEmpty
    Layout.alignment: Qt.AlignVCenter

    property bool hasTrack: false
    property string trackName: ""

    HoverHandler {
        id: hover
    }

    ToolTip.visible: hover.hovered && indicator.trackName.length > 0
    ToolTip.text: indicator.trackName
}
