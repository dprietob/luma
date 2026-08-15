import QtQuick
import QtQml.Models

Rectangle {
    id: mixerArea
    color: Theme.bgPanel

    property bool reorderMode: false
    property bool gridMode: false

    readonly property int gridRows: gridMode ? 2 : 1
    readonly property int cellGap: Theme.spacingS

    SequentialAnimation {
        running: mixerArea.reorderMode
        loops: Animation.Infinite
        ColorAnimation {
            target: mixerArea
            property: "color"
            from: Theme.bgPanel
            to: Theme.accent
            duration: 600
        }
        ColorAnimation {
            target: mixerArea
            property: "color"
            from: Theme.accent
            to: Theme.bgPanel
            duration: 600
        }
        onStopped: mixerArea.color = Theme.bgPanel
    }

    ChannelStrip {
        id: sizer
        visible: false
        channel: audioEngine.channelAt(0)
    }

    GridView {
        id: channelsView
        anchors.fill: parent
        anchors.margins: Theme.spacingM
        clip: true
        flow: GridView.FlowTopToBottom
        interactive: !mixerArea.reorderMode

        cellWidth: sizer.implicitWidth + mixerArea.cellGap
        cellHeight: Math.floor(height / mixerArea.gridRows)

        Component.onCompleted: channelsView.applyPersistedOrder()

        function channelIdAt(i) {
            const m = visualModel.items.get(i).model;
            const ch = m.modelData !== undefined ? m.modelData : m;
            return ch.id;
        }

        function commitOrder() {
            let ids = [];
            for (let i = 0; i < visualModel.items.count; ++i)
                ids.push(channelsView.channelIdAt(i));
            audioEngine.setChannelOrder(ids);
        }

        function applyPersistedOrder() {
            const order = audioEngine.channelOrder();
            if (!order || order.length !== visualModel.items.count)
                return;
            for (let target = 0; target < order.length; ++target) {
                for (let i = target; i < visualModel.items.count; ++i) {
                    if (channelsView.channelIdAt(i) === order[target]) {
                        if (i !== target)
                            visualModel.items.move(i, target);
                        break;
                    }
                }
            }
        }

        model: DelegateModel {
            id: visualModel
            model: audioEngine.channels

            delegate: Item {
                id: slot

                required property var modelData
                readonly property int visualIndex: DelegateModel.itemsIndex

                width: channelsView.cellWidth
                height: channelsView.cellHeight

                ChannelStrip {
                    id: content
                    channel: slot.modelData
                    width: slot.width - mixerArea.cellGap
                    height: slot.height - mixerArea.cellGap
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter

                    Drag.active: dragOverlay.drag.active
                    Drag.source: slot
                    Drag.keys: ["channelReorder"]
                    Drag.hotSpot.x: width / 2
                    Drag.hotSpot.y: height / 2

                    states: State {
                        when: content.Drag.active
                        ParentChange {
                            target: content
                            parent: channelsView
                        }
                        AnchorChanges {
                            target: content
                            anchors.horizontalCenter: undefined
                            anchors.verticalCenter: undefined
                        }
                    }
                }

                DropArea {
                    anchors.fill: parent
                    keys: ["channelReorder"]
                    onEntered: drag => {
                        const from = drag.source.visualIndex;
                        if (from !== slot.visualIndex)
                            visualModel.items.move(from, slot.visualIndex);
                    }
                }

                MouseArea {
                    id: dragOverlay
                    anchors.fill: parent
                    enabled: mixerArea.reorderMode
                    visible: mixerArea.reorderMode
                    cursorShape: Qt.SizeAllCursor
                    drag.target: content
                    drag.axis: Drag.XAndYAxis
                    onReleased: channelsView.commitOrder()
                }
            }
        }

        displaced: Transition {
            enabled: mixerArea.reorderMode
            NumberAnimation {
                properties: "x,y"
                duration: 150
            }
        }
    }
}
