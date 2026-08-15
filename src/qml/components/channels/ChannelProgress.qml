import QtQuick
import QtQuick.Layouts

Rectangle {
    id: progress
    color: Theme.borderMid
    height: 4
    Layout.fillWidth: true
    Layout.topMargin: Theme.spacingS

    required property var channel
    signal clicked

    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: parent.width * progress.channel.progress
        color: Theme.trackLoaded
    }

    Rectangle {
        visible: progress.channel.timeline.regionEnabled && progress.channel.timeline.hasStart
        x: progress.channel.timeline.start * parent.width - width / 2
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 2
        color: Theme.accent
    }

    Rectangle {
        visible: progress.channel.timeline.regionEnabled && progress.channel.timeline.hasEnd
        x: progress.channel.timeline.end * parent.width - width / 2
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 2
        color: Theme.accent
    }

    MouseArea {
        anchors.fill: parent
        enabled: progress.channel.hasTrack
        cursorShape: Qt.PointingHandCursor
        onClicked: progress.clicked()
    }
}
