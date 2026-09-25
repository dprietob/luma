import QtQuick

Item {
    id: hvu

    property real levelLeft: 0.0
    property real levelRight: 0.0

    Column {
        anchors.fill: parent
        spacing: Theme.spacingXS
        HVuBar {
            width: parent.width
            height: (hvu.height - Theme.spacingXS) / 2
            level: hvu.levelLeft
        }
        HVuBar {
            width: parent.width
            height: (hvu.height - Theme.spacingXS) / 2
            level: hvu.levelRight
        }
    }

    component HVuBar: Rectangle {
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
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 1
            width: (parent.width - 2) * Math.min(parent.clampedLevel, 0.70)
            color: Theme.vuGreen
        }

        Rectangle {
            x: 1 + (parent.width - 2) * 0.70
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 1
            width: (parent.width - 2) * Math.max(0, Math.min(parent.clampedLevel, 0.90) - 0.70)
            visible: parent.clampedLevel > 0.70
            color: Theme.vuYellow
        }

        Rectangle {
            x: 1 + (parent.width - 2) * 0.90
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.margins: 1
            width: (parent.width - 2) * Math.max(0, parent.clampedLevel - 0.90)
            visible: parent.clampedLevel > 0.90
            color: Theme.vuRed
        }
    }
}
