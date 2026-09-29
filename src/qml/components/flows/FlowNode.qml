import QtQuick

Item {
    id: node

    required property string nodeId
    property int channelId: -1
    property string label: ""
    property bool isStart: false
    property bool inputHighlighted: false
    property int canvasWidth: 0
    property int canvasHeight: 0

    width: 140
    height: 56

    signal moved
    signal deleteRequested
    signal configRequested
    signal edgeStartRequested(real cx, real cy)
    signal edgeDragging(real cx, real cy)
    signal edgeDropped(real cx, real cy)

    onXChanged: node.moved()
    onYChanged: node.moved()

    function clampPosition() {
        if (node.canvasWidth > 0)
            node.x = Math.max(0, Math.min(node.canvasWidth - node.width, node.x));
        if (node.canvasHeight > 0)
            node.y = Math.max(0, Math.min(node.canvasHeight - node.height, node.y));
    }

    function outputCenter() {
        return node.mapToItem(node.parent, node.width, node.height / 2);
    }

    Rectangle {
        anchors.fill: parent
        radius: Theme.radiusXS
        color: node.isStart ? Theme.bgHover : Theme.bgSubPanel
        border.color: node.isStart ? Theme.accent : Theme.borderBright
        border.width: node.isStart ? 2 : 1

        MouseArea {
            id: bodyDrag
            anchors.fill: parent
            drag.target: node
            drag.threshold: 0
            cursorShape: Qt.SizeAllCursor
            onReleased: node.clampPosition()
            onDoubleClicked: if (!node.isStart)
                node.configRequested()
        }

        Text {
            anchors.fill: parent
            anchors.margins: Theme.spacingM
            text: node.label
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
            font.bold: node.isStart
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        Text {
            visible: !node.isStart
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: Theme.spacingXS
            text: "✕"
            color: delMouse.containsMouse ? Theme.accent : Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall

            MouseArea {
                id: delMouse
                anchors.fill: parent
                anchors.margins: -Theme.spacingXS
                hoverEnabled: true
                preventStealing: true
                cursorShape: Qt.PointingHandCursor
                onClicked: node.deleteRequested()
            }
        }
    }

    Rectangle {
        id: inPort
        visible: !node.isStart
        anchors.verticalCenter: parent.verticalCenter
        x: -width / 2
        width: 16
        height: 16
        radius: 8
        color: Theme.borderBright
        border.color: (node.inputHighlighted || inHover.hovered) ? Theme.accent : Theme.bgPanel
        border.width: 2

        HoverHandler {
            id: inHover
        }
    }

    Rectangle {
        id: outPort
        anchors.verticalCenter: parent.verticalCenter
        x: node.width - width / 2
        width: 18
        height: 18
        radius: 9
        color: portMouse.pressed ? Theme.accent : Theme.borderBright
        border.color: (portMouse.pressed || portMouse.containsMouse) ? Theme.accent : Theme.bgPanel
        border.width: 2

        MouseArea {
            id: portMouse
            anchors.fill: parent
            anchors.margins: -10
            hoverEnabled: true
            preventStealing: true
            cursorShape: Qt.CrossCursor
            onPressed: mouse => {
                const c = node.outputCenter();
                node.edgeStartRequested(c.x, c.y);
            }
            onPositionChanged: mouse => {
                const p = portMouse.mapToItem(node.parent, mouse.x, mouse.y);
                node.edgeDragging(p.x, p.y);
            }
            onReleased: mouse => {
                const p = portMouse.mapToItem(node.parent, mouse.x, mouse.y);
                node.edgeDropped(p.x, p.y);
            }
        }
    }
}
