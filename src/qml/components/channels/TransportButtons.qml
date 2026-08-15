import QtQuick
import QtQuick.Layouts

RowLayout {
    id: transport
    spacing: Theme.spacingS
    Layout.alignment: Qt.AlignHCenter

    property bool isPlaying: false
    property bool isPaused: false

    signal playPauseRequested
    signal stopRequested

    CButton {
        id: playButton
        iconName: transport.isPlaying ? "pause" : "play"
        square: true
        blinking: transport.isPaused
        bgColor: transport.isPaused ? Theme.accent : (transport.isPlaying ? Theme.accent : "transparent")
        onClicked: transport.playPauseRequested()
    }

    CButton {
        id: stopButton
        iconName: "stop"
        square: true
        danger: true
        onClicked: transport.stopRequested()
    }
}
