import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: editor
    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    padding: Theme.spacingL
    closePolicy: Popup.CloseOnEscape

    width: Math.min(Overlay.overlay ? Overlay.overlay.width - 80 : 1000, 1080)
    height: Math.min(Overlay.overlay ? Overlay.overlay.height - 80 : 620, 680)

    property string flowId: ""
    property string flowName: ""
    property int editorPreset: 0

    property var nodeList: []
    property var edgeList: []
    property int nextNodeSeq: 1
    property int nextEdgeSeq: 1
    property int rev: 0

    property bool tempActive: false
    property real tx1: 0
    property real ty1: 0
    property real tx2: 0
    property real ty2: 0
    property string edgeSourceId: ""
    property string hoverTargetId: ""

    property real panX: 0
    property real panY: 0
    property real zoom: 1
    readonly property real minZoom: 0.35
    readonly property real maxZoom: 2.5

    function channelLabel(id) {
        if (editor.editorPreset === presetManager.activePreset) {
            const ch = audioEngine.channelAt(id);
            if (ch)
                return ch.name;
        }
        return "CH " + (id + 1 < 10 ? "0" + (id + 1) : "" + (id + 1));
    }

    function load(id) {
        editor.flowId = id;
        const data = flowManager.flowData(id);
        editor.flowName = data.name !== undefined ? data.name : "";
        editor.editorPreset = data.preset !== undefined ? data.preset : 0;

        const loadedNodes = [];
        let maxN = 0;
        const nodes = data.nodes !== undefined ? data.nodes : [];
        for (let i = 0; i < nodes.length; ++i) {
            const n = nodes[i];
            loadedNodes.push({
                "id": n.id,
                "channelId": n.channelId,
                "x": n.x !== undefined ? n.x : 0,
                "y": n.y !== undefined ? n.y : 0,
                "config": n.config !== undefined ? n.config : ({})
            });
            const num = parseInt(String(n.id).replace("n", ""));
            if (!isNaN(num) && num > maxN)
                maxN = num;
        }
        editor.nodeList = loadedNodes;
        editor.nextNodeSeq = maxN + 1;

        const loadedEdges = [];
        let maxE = 0;
        const edges = data.edges !== undefined ? data.edges : [];
        for (let i = 0; i < edges.length; ++i) {
            const e = edges[i];
            const trig = e.trigger !== undefined ? e.trigger : {};
            loadedEdges.push({
                "id": e.id !== undefined ? e.id : "e" + (i + 1),
                "from": e.from,
                "to": e.to,
                "triggerType": trig.type !== undefined ? trig.type : "immediate",
                "triggerSeconds": trig.seconds !== undefined ? trig.seconds : 0,
                "originAction": e.originAction !== undefined ? e.originAction : "none",
                "targetAction": e.targetAction !== undefined ? e.targetAction : "play"
            });
            const num = parseInt(String(e.id).replace("e", ""));
            if (!isNaN(num) && num > maxE)
                maxE = num;
        }
        editor.edgeList = loadedEdges;
        editor.nextEdgeSeq = maxE + 1;

        startNode.x = data.startX !== undefined ? data.startX : 30;
        startNode.y = data.startY !== undefined ? data.startY : 220;
        editor.tempActive = false;
        editor.resetView();
        Qt.callLater(editor.refreshEdges);
    }

    function save() {
        const nodes = [];
        for (let i = 0; i < editor.nodeList.length; ++i) {
            const n = editor.nodeList[i];
            nodes.push({
                "id": n.id,
                "channelId": n.channelId,
                "x": Math.round(n.x),
                "y": Math.round(n.y),
                "config": n.config !== undefined ? n.config : ({})
            });
        }
        const edges = [];
        for (let i = 0; i < editor.edgeList.length; ++i) {
            const e = editor.edgeList[i];
            edges.push({
                "id": e.id,
                "from": e.from,
                "to": e.to,
                "trigger": {
                    "type": e.triggerType,
                    "seconds": e.triggerSeconds
                },
                "originAction": e.originAction,
                "targetAction": e.targetAction
            });
        }
        const data = flowManager.flowData(editor.flowId);
        data.name = editor.flowName;
        data.preset = editor.editorPreset;
        data.nodes = nodes;
        data.edges = edges;
        data.startX = Math.round(startNode.x);
        data.startY = Math.round(startNode.y);
        flowManager.updateFlow(editor.flowId, data);
        editor.close();
    }

    function addNode(channelId) {
        const stagger = (editor.nodeList.length % 6) * 24;
        const cx = (canvas.width / 2 - editor.panX) / editor.zoom - 70 + stagger;
        const cy = (canvas.height / 2 - editor.panY) / editor.zoom - 28 + stagger;
        const nx = Math.max(0, Math.min(flowContent.width - 140, cx));
        const ny = Math.max(0, Math.min(flowContent.height - 56, cy));
        const id = "n" + editor.nextNodeSeq++;
        editor.nodeList = editor.nodeList.concat([
            {
                "id": id,
                "channelId": channelId,
                "x": nx,
                "y": ny,
                "config": ({})
            }
        ]);
        Qt.callLater(editor.refreshEdges);
    }

    function nodeById(id) {
        for (let i = 0; i < editor.nodeList.length; ++i) {
            if (editor.nodeList[i].id === id)
                return editor.nodeList[i];
        }
        return null;
    }

    function openNodeConfig(id) {
        const n = editor.nodeById(id);
        if (!n)
            return;
        flowNodeConfig.loadFromMap(n.config !== undefined ? n.config : ({}));
        nodeConfig.loadNode(id, editor.channelLabel(n.channelId));
    }

    function applyNodeConfig(id) {
        const n = editor.nodeById(id);
        if (n)
            n.config = flowNodeConfig.toMap();
    }

    function removeNode(id) {
        editor.nodeList = editor.nodeList.filter(n => n.id !== id);
        editor.edgeList = editor.edgeList.filter(e => e.from !== id && e.to !== id);
        Qt.callLater(editor.refreshEdges);
    }

    function addEdge(from, to) {
        if (from === to || to === "start")
            return;
        const id = "e" + editor.nextEdgeSeq++;
        editor.edgeList = editor.edgeList.concat([
            {
                "id": id,
                "from": from,
                "to": to,
                "triggerType": from === "start" ? "immediate" : "elapsed",
                "triggerSeconds": 0,
                "originAction": "none",
                "targetAction": "play"
            }
        ]);
        Qt.callLater(editor.refreshEdges);
    }

    function removeEdge(id) {
        editor.edgeList = editor.edgeList.filter(e => e.id !== id);
        Qt.callLater(editor.refreshEdges);
    }

    function edgeById(id) {
        for (let i = 0; i < editor.edgeList.length; ++i) {
            if (editor.edgeList[i].id === id)
                return editor.edgeList[i];
        }
        return null;
    }

    function updateEdge(id, patch) {
        editor.edgeList = editor.edgeList.map(e => {
            if (e.id !== id)
                return e;
            const merged = {
                "id": e.id,
                "from": e.from,
                "to": e.to,
                "triggerType": e.triggerType,
                "triggerSeconds": e.triggerSeconds,
                "originAction": e.originAction,
                "targetAction": e.targetAction
            };
            for (const k in patch)
                merged[k] = patch[k];
            return merged;
        });
        Qt.callLater(editor.refreshEdges);
    }

    function edgeSummary(e) {
        const origin = e.originAction === "fadeOut" ? "↘ " : (e.originAction === "stop" ? "■ " : "");
        const target = e.targetAction === "fadeIn" ? "↗" : "▶";
        let when = "";
        if (e.triggerType === "elapsed")
            when = " " + e.triggerSeconds + "s";
        else if (e.triggerType === "finish")
            when = " end";
        return origin + target + when;
    }

    function openEdgeConfig(id) {
        const e = editor.edgeById(id);
        if (!e)
            return;
        edgeConfig.loadEdge(id, e.from === "start", e.triggerType, e.triggerSeconds, e.originAction, e.targetAction);
    }

    function beginEdge(sourceId, cx, cy) {
        editor.edgeSourceId = sourceId;
        editor.tempActive = true;
        editor.tx1 = cx;
        editor.ty1 = cy;
        editor.tx2 = cx;
        editor.ty2 = cy;
        editor.hoverTargetId = "";
        editor.refreshEdges();
    }

    function dragEdge(cx, cy) {
        editor.tx2 = cx;
        editor.ty2 = cy;
        const target = editor.nodeAt(cx, cy);
        editor.hoverTargetId = target !== editor.edgeSourceId ? target : "";
        editor.refreshEdges();
    }

    function dropEdge(cx, cy) {
        const target = editor.nodeAt(cx, cy);
        if (target !== "" && target !== editor.edgeSourceId)
            editor.addEdge(editor.edgeSourceId, target);
        editor.tempActive = false;
        editor.hoverTargetId = "";
        editor.refreshEdges();
    }

    function outputPoint(id) {
        if (id === "start")
            return Qt.point(startNode.x + startNode.width, startNode.y + startNode.height / 2);
        for (let i = 0; i < nodesRepeater.count; ++i) {
            const it = nodesRepeater.itemAt(i);
            if (it && it.nodeId === id)
                return Qt.point(it.x + it.width, it.y + it.height / 2);
        }
        return null;
    }

    function inputPoint(id) {
        if (id === "start")
            return Qt.point(startNode.x, startNode.y + startNode.height / 2);
        for (let i = 0; i < nodesRepeater.count; ++i) {
            const it = nodesRepeater.itemAt(i);
            if (it && it.nodeId === id)
                return Qt.point(it.x, it.y + it.height / 2);
        }
        return null;
    }

    function nodeAt(x, y) {
        const pad = 16;
        for (let i = 0; i < nodesRepeater.count; ++i) {
            const it = nodesRepeater.itemAt(i);
            if (it && x >= it.x - pad && x <= it.x + it.width + pad && y >= it.y - pad && y <= it.y + it.height + pad)
                return it.nodeId;
        }
        return "";
    }

    function refreshEdges() {
        edgeCanvas.requestPaint();
        editor.rev++;
    }

    function panBy(dx, dy) {
        editor.panX += dx;
        editor.panY += dy;
    }

    function applyZoom(factor, cx, cy) {
        const z0 = editor.zoom;
        const z1 = Math.max(editor.minZoom, Math.min(editor.maxZoom, z0 * factor));
        if (z1 === z0)
            return;
        const ratio = z1 / z0;
        editor.panX = cx - (cx - editor.panX) * ratio;
        editor.panY = cy - (cy - editor.panY) * ratio;
        editor.zoom = z1;
    }

    function resetView() {
        editor.panX = 0;
        editor.panY = 0;
        editor.zoom = 1;
    }

    background: Rectangle {
        color: Theme.bgPanel
        border.color: Theme.borderBright
        border.width: 2
        radius: Theme.radiusXS
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingL

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingM

            Text {
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                text: qsTr("Flow editor — %1").arg(editor.flowName)
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMid
                font.bold: true
                elide: Text.ElideRight
            }

            Text {
                text: qsTr("Preset")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
            }

            CButton {
                iconName: "left"
                Layout.preferredWidth: 30
                visible: editor.nodeList.length === 0
                enabled: editor.editorPreset > 0
                onClicked: editor.editorPreset = Math.max(0, editor.editorPreset - 1)
            }

            Text {
                text: qsTr("P%1").arg(editor.editorPreset + 1)
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
                horizontalAlignment: Text.AlignHCenter
                Layout.preferredWidth: 40
            }

            CButton {
                iconName: "right"
                Layout.preferredWidth: 30
                visible: editor.nodeList.length === 0
                enabled: editor.editorPreset < presetManager.count - 1
                onClicked: editor.editorPreset = Math.min(presetManager.count - 1, editor.editorPreset + 1)
            }

            CButton {
                symbol: qsTr("Save")
                Layout.leftMargin: Theme.spacingL
                onClicked: editor.save()
            }

            CButton {
                symbol: qsTr("Cancel")
                onClicked: editor.close()
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: Theme.spacingM

            Rectangle {
                Layout.preferredWidth: 190
                Layout.fillHeight: true
                color: Theme.bgSubPanel
                border.color: Theme.borderSubtle
                border.width: 1
                radius: Theme.radiusXS

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingS
                    spacing: Theme.spacingS

                    Text {
                        text: qsTr("Channels")
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeBase
                        font.bold: true
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Click a channel to add it to the canvas.")
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeSmall
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        Layout.fillWidth: true
                        text: qsTr("Scroll or drag the canvas to pan.")
                        color: Theme.textMuted
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeSmall
                        wrapMode: Text.WordWrap
                    }

                    ListView {
                        id: paletteView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: Theme.spacingXS
                        model: audioEngine.channelCount
                        boundsBehavior: Flickable.StopAtBounds

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }

                        delegate: Rectangle {
                            id: palItem
                            required property int index
                            width: ListView.view.width
                            height: 28
                            radius: Theme.radiusXS
                            color: palMouse.pressed ? Theme.accent : (palMouse.containsMouse ? Theme.bgHover : Theme.bgControl)
                            border.color: Theme.borderSubtle
                            border.width: 1

                            Text {
                                anchors.fill: parent
                                anchors.margins: Theme.spacingS
                                text: editor.channelLabel(palItem.index)
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeBase
                                verticalAlignment: Text.AlignVCenter
                                elide: Text.ElideRight
                            }

                            MouseArea {
                                id: palMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: editor.addNode(palItem.index)
                            }
                        }
                    }
                }
            }

            Rectangle {
                id: canvas
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                color: Theme.bgBase
                border.color: Theme.borderSubtle
                border.width: 1
                radius: Theme.radiusXS

                MouseArea {
                    id: panArea
                    anchors.fill: parent
                    cursorShape: panArea.pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    property real lastX: 0
                    property real lastY: 0
                    onPressed: mouse => {
                        panArea.lastX = mouse.x;
                        panArea.lastY = mouse.y;
                    }
                    onPositionChanged: mouse => {
                        editor.panBy(mouse.x - panArea.lastX, mouse.y - panArea.lastY);
                        panArea.lastX = mouse.x;
                        panArea.lastY = mouse.y;
                    }
                }

                WheelHandler {
                    id: canvasWheel
                    onWheel: event => {
                        if (event.modifiers & Qt.ControlModifier) {
                            const factor = event.angleDelta.y > 0 ? 1.15 : 1 / 1.15;
                            editor.applyZoom(factor, canvasWheel.point.position.x, canvasWheel.point.position.y);
                        } else if (event.modifiers & Qt.ShiftModifier) {
                            editor.panBy(event.angleDelta.y, event.angleDelta.x);
                        } else {
                            editor.panBy(event.angleDelta.x, event.angleDelta.y);
                        }
                    }
                }

                Item {
                    id: flowContent
                    x: editor.panX
                    y: editor.panY
                    width: Math.max(canvas.width, 2400)
                    height: Math.max(canvas.height, 1600)
                    transformOrigin: Item.TopLeft
                    scale: editor.zoom

                    Canvas {
                        id: edgeCanvas
                        anchors.fill: parent

                        onPaint: {
                            const ctx = getContext("2d");
                            ctx.reset();
                            ctx.lineWidth = 2;
                            ctx.strokeStyle = Theme.borderBright;
                            for (let i = 0; i < editor.edgeList.length; ++i) {
                                const e = editor.edgeList[i];
                                const a = editor.outputPoint(e.from);
                                const b = editor.inputPoint(e.to);
                                if (!a || !b)
                                    continue;
                                ctx.beginPath();
                                ctx.moveTo(a.x, a.y);
                                ctx.lineTo(b.x, b.y);
                                ctx.stroke();
                            }
                            if (editor.tempActive) {
                                ctx.strokeStyle = Theme.accent;
                                ctx.setLineDash([6, 4]);
                                ctx.beginPath();
                                ctx.moveTo(editor.tx1, editor.ty1);
                                ctx.lineTo(editor.tx2, editor.ty2);
                                ctx.stroke();
                                ctx.setLineDash([]);
                            }
                        }
                    }

                    FlowNode {
                        id: startNode
                        nodeId: "start"
                        label: qsTr("START")
                        isStart: true
                        canvasWidth: flowContent.width
                        canvasHeight: flowContent.height
                        onMoved: editor.refreshEdges()
                        onEdgeStartRequested: (cx, cy) => editor.beginEdge("start", cx, cy)
                        onEdgeDragging: (cx, cy) => editor.dragEdge(cx, cy)
                        onEdgeDropped: (cx, cy) => editor.dropEdge(cx, cy)
                    }

                    Repeater {
                        id: nodesRepeater
                        model: editor.nodeList

                        delegate: FlowNode {
                            id: nodeDelegate
                            required property var modelData
                            required property int index
                            nodeId: modelData.id
                            channelId: modelData.channelId
                            label: editor.channelLabel(modelData.channelId)
                            inputHighlighted: editor.tempActive && editor.hoverTargetId === nodeDelegate.nodeId
                            canvasWidth: flowContent.width
                            canvasHeight: flowContent.height
                            Component.onCompleted: {
                                nodeDelegate.x = modelData.x;
                                nodeDelegate.y = modelData.y;
                            }
                            onMoved: {
                                if (nodeDelegate.index >= 0 && nodeDelegate.index < editor.nodeList.length) {
                                    editor.nodeList[nodeDelegate.index].x = nodeDelegate.x;
                                    editor.nodeList[nodeDelegate.index].y = nodeDelegate.y;
                                }
                                editor.refreshEdges();
                            }
                            onDeleteRequested: editor.removeNode(nodeDelegate.nodeId)
                            onConfigRequested: editor.openNodeConfig(nodeDelegate.nodeId)
                            onEdgeStartRequested: (cx, cy) => editor.beginEdge(nodeDelegate.nodeId, cx, cy)
                            onEdgeDragging: (cx, cy) => editor.dragEdge(cx, cy)
                            onEdgeDropped: (cx, cy) => editor.dropEdge(cx, cy)
                        }
                    }

                    Repeater {
                        model: editor.edgeList

                        delegate: Item {
                            id: edgeMarker
                            required property var modelData
                            readonly property point mid: {
                                editor.rev;
                                const a = editor.outputPoint(modelData.from);
                                const b = editor.inputPoint(modelData.to);
                                return (a && b) ? Qt.point((a.x + b.x) / 2, (a.y + b.y) / 2) : Qt.point(-100, -100);
                            }
                            x: mid.x - width / 2
                            y: mid.y - height / 2
                            width: Math.max(24, summaryText.implicitWidth + 12)
                            height: 20
                            visible: mid.x >= 0

                            Rectangle {
                                anchors.fill: parent
                                radius: Theme.radiusXS
                                color: markerMouse.containsMouse ? Theme.bgHover : Theme.bgControl
                                border.color: Theme.accent
                                border.width: 1

                                Text {
                                    id: summaryText
                                    anchors.centerIn: parent
                                    text: editor.edgeSummary(edgeMarker.modelData)
                                    color: Theme.textPrimary
                                    font.family: Theme.fontFamily
                                    font.pixelSize: Theme.fontSizeSmall
                                }
                            }

                            MouseArea {
                                id: markerMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: editor.openEdgeConfig(edgeMarker.modelData.id)
                            }
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: editor.nodeList.length === 0
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("Drag channels here and connect them\nfrom the start point.")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBase
                }

                Row {
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: Theme.spacingS
                    spacing: Theme.spacingXS

                    CButton {
                        symbol: "−"
                        fontSmall: true
                        onClicked: editor.applyZoom(1 / 1.15, canvas.width / 2, canvas.height / 2)
                    }
                    CButton {
                        symbol: Math.round(editor.zoom * 100) + "%"
                        fontSmall: true
                        onClicked: editor.resetView()
                    }
                    CButton {
                        symbol: "+"
                        fontSmall: true
                        onClicked: editor.applyZoom(1.15, canvas.width / 2, canvas.height / 2)
                    }
                }
            }
        }
    }

    FlowEdgeConfigDialog {
        id: edgeConfig
        onEdited: (id, patch) => editor.updateEdge(id, patch)
        onDeleted: id => editor.removeEdge(id)
    }

    FlowNodeConfigDialog {
        id: nodeConfig
        onApplied: id => editor.applyNodeConfig(id)
    }

    onClosed: {
        editor.tempActive = false;
        edgeConfig.close();
        nodeConfig.close();
    }
}
