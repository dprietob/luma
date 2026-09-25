import QtQuick
import QtQuick.Controls.Basic

Slider {
    id: vSlider
    orientation: Qt.Vertical
    from: 0.0
    to: 1.0
    stepSize: 0.01
    implicitWidth: vSlider.showMarks ? 46 : 22
    implicitHeight: 100

    property bool showMarks: true

    property bool rangeHandles: false
    property real rangeMax: 1.0
    property real rangeMin: 0.0

    signal resetRequested
    signal rangeMaxMoved(real value)
    signal rangeMinMoved(real value)

    readonly property var marks: [
        {
            "label": "100",
            "frac": 1.0
        },
        {
            "label": "90",
            "frac": 0.90
        },
        {
            "label": "80",
            "frac": 0.80
        },
        {
            "label": "70",
            "frac": 0.70
        },
        {
            "label": "60",
            "frac": 0.60
        },
        {
            "label": "50",
            "frac": 0.50
        },
        {
            "label": "40",
            "frac": 0.40
        },
        {
            "label": "30",
            "frac": 0.30
        },
        {
            "label": "20",
            "frac": 0.20
        },
        {
            "label": "10",
            "frac": 0.10
        },
        {
            "label": "0",
            "frac": 0.0
        }
    ]

    background: Item {
        id: bg
        anchors.fill: parent

        Rectangle {
            id: track
            x: vSlider.showMarks ? vSlider.leftPadding + 10 : vSlider.leftPadding + (vSlider.availableWidth - width) / 2
            y: vSlider.topPadding
            width: 6
            height: vSlider.availableHeight
            color: Theme.controlTrack
            border.color: Theme.bgSubPanel
            border.width: 1

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                width: parent.width - 2 * parent.border.width
                height: vSlider.position * parent.height
                color: Theme.accent
            }
        }

        Repeater {
            model: vSlider.showMarks ? vSlider.marks : []
            delegate: Row {
                required property var modelData
                readonly property bool isFull: modelData.frac === 1.0
                spacing: 3
                x: track.x + track.width + 3
                y: vSlider.topPadding + (1.0 - modelData.frac) * vSlider.availableHeight - height / 2

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.isFull ? 8 : 5
                    height: 1
                    color: Theme.textSecondary
                }
                Text {
                    text: parent.modelData.label
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBase
                }
            }
        }
    }

    handle: Image {
        x: track.x + track.width / 2 - width / 2
        y: vSlider.topPadding + vSlider.visualPosition * vSlider.availableHeight - height / 2
        width: 20
        height: 35
        source: "qrc:/icons/thumb.svg"
        sourceSize: Qt.size(width, height)
    }

    Item {
        id: maxHandle
        z: 10
        visible: vSlider.rangeHandles
        x: track.x - 8
        width: track.width + 8
        height: 18
        y: vSlider.topPadding + (1.0 - vSlider.rangeMax) * vSlider.availableHeight - height / 2

        function apply(mouse) {
            const pt = maxHandle.mapToItem(vSlider, 0, mouse.y);
            let f = 1.0 - (pt.y - vSlider.topPadding) / vSlider.availableHeight;
            f = Math.max(vSlider.rangeMin, Math.min(1.0, f));
            vSlider.rangeMaxMoved(f);
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: 2
            color: vSlider.rangeMax < 0.999 ? Theme.accent : Theme.borderBright
        }

        Rectangle {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 10
            height: 10
            radius: Theme.radiusXS
            color: vSlider.rangeMax < 0.999 ? Theme.accent : Theme.borderBright
        }

        MouseArea {
            anchors.fill: parent
            preventStealing: true
            cursorShape: Qt.SizeVerCursor
            onPressed: mouse => maxHandle.apply(mouse)
            onPositionChanged: mouse => maxHandle.apply(mouse)
            onDoubleClicked: vSlider.rangeMaxMoved(1.0)
        }
    }

    Item {
        id: minHandle
        z: 10
        visible: vSlider.rangeHandles
        x: track.x - 8
        width: track.width + 8
        height: 18
        y: vSlider.topPadding + (1.0 - vSlider.rangeMin) * vSlider.availableHeight - height / 2

        function apply(mouse) {
            const pt = minHandle.mapToItem(vSlider, 0, mouse.y);
            let f = 1.0 - (pt.y - vSlider.topPadding) / vSlider.availableHeight;
            f = Math.max(0.0, Math.min(vSlider.rangeMax, f));
            vSlider.rangeMinMoved(f);
        }

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: 2
            color: vSlider.rangeMin > 0.001 ? Theme.accent : Theme.borderBright
        }

        Rectangle {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 10
            height: 10
            radius: Theme.radiusXS
            color: vSlider.rangeMin > 0.001 ? Theme.accent : Theme.borderBright
        }

        MouseArea {
            anchors.fill: parent
            preventStealing: true
            cursorShape: Qt.SizeVerCursor
            onPressed: mouse => minHandle.apply(mouse)
            onPositionChanged: mouse => minHandle.apply(mouse)
            onDoubleClicked: vSlider.rangeMinMoved(0.0)
        }
    }
}
