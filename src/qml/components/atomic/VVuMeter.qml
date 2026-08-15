import QtQuick

Item {
    id: vu
    implicitWidth: 18

    property real levelLeft: 0.0
    property real levelRight: 0.0

    Row {
        anchors.fill: parent
        spacing: Theme.spacingXS
        VuBar {
            width: (vu.width - Theme.spacingXS) / 2
            height: parent.height
            level: vu.levelLeft
        }
        VuBar {
            width: (vu.width - Theme.spacingXS) / 2
            height: parent.height
            level: vu.levelRight
        }
    }

    component VuBar: Rectangle {
        id: bar
        property real level: 0.0

        readonly property real clampedLevel: Math.max(0, Math.min(1, level))
        readonly property real greenTop: 0.70
        readonly property real yellowTop: 0.90

        color: Theme.vuBackground
        border.color: Theme.borderSubtle
        border.width: 1

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 1
            height: (parent.height - 2) * Math.min(bar.clampedLevel, bar.greenTop)
            color: Theme.vuGreen
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 1
            y: 1 + (parent.height - 2) * (1.0 - Math.min(bar.clampedLevel, bar.yellowTop))
            height: (parent.height - 2) * Math.max(0, Math.min(bar.clampedLevel, bar.yellowTop) - bar.greenTop)
            visible: bar.clampedLevel > bar.greenTop
            color: Theme.vuYellow
        }

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 1
            y: 1 + (parent.height - 2) * (1.0 - bar.clampedLevel)
            height: (parent.height - 2) * Math.max(0, bar.clampedLevel - bar.yellowTop)
            visible: bar.clampedLevel > bar.yellowTop
            color: Theme.vuRed
        }
    }
}
