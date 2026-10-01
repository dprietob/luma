import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: flowListPanel
    color: Theme.bgPanel

    FlowEditorDialog {
        id: flowEditor
    }

    Shortcut {
        sequence: (appSettings.shortcutsRevision, appSettings.shortcut("newFlow"))
        enabled: !appSettings.capturing
        onActivated: {
            flowEditor.load(flowManager.createFlow());
            flowEditor.open();
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingM
        spacing: Theme.spacingS

        RowLayout {
            Layout.fillWidth: true

            Text {
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                text: qsTr("Flow list")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMid
                font.bold: true
            }

            Item {
                Layout.fillWidth: true
            }

            CButton {
                symbol: qsTr("New flow")
                onClicked: {
                    flowEditor.load(flowManager.createFlow());
                    flowEditor.open();
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: Theme.spacingS
            Layout.bottomMargin: Theme.spacingM
            color: Theme.borderMid
        }

        ListView {
            id: flowView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: Theme.spacingXS
            model: flowManager.flows
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            Text {
                anchors.centerIn: parent
                visible: flowView.count === 0
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("No flows yet.\nPress “New flow” to create one.")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
            }

            delegate: Rectangle {
                id: flowItem
                required property var modelData
                readonly property bool isRunning: flowManager.running && flowManager.runningFlowId === flowItem.modelData.id

                width: ListView.view.width
                height: 42
                color: flowItem.isRunning ? Theme.bgHover : Theme.bgSubPanel
                border.color: Theme.borderSubtle
                border.width: 1
                radius: Theme.radiusXS

                SequentialAnimation {
                    running: flowItem.isRunning
                    loops: Animation.Infinite
                    onStopped: flowItem.border.color = Theme.borderSubtle
                    ColorAnimation {
                        target: flowItem
                        property: "border.color"
                        from: Theme.borderSubtle
                        to: Theme.accent
                        duration: 500
                    }
                    ColorAnimation {
                        target: flowItem
                        property: "border.color"
                        from: Theme.accent
                        to: Theme.borderSubtle
                        duration: 500
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingS
                    spacing: Theme.spacingS

                    Item {
                        id: nameBox
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        property bool editing: false

                        Text {
                            anchors.fill: parent
                            visible: !nameBox.editing
                            text: flowItem.modelData.name
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeBase
                            verticalAlignment: Text.AlignVCenter
                            elide: Text.ElideRight

                            TapHandler {
                                onDoubleTapped: nameBox.editing = true
                            }
                        }

                        CTextField {
                            id: nameField
                            anchors.fill: parent
                            visible: nameBox.editing
                            onAccepted: commit()
                            onActiveFocusChanged: if (!activeFocus && nameBox.editing)
                                commit()

                            function commit() {
                                flowManager.renameFlow(flowItem.modelData.id, text);
                                nameBox.editing = false;
                            }
                        }

                        onEditingChanged: {
                            if (!editing)
                                return;
                            nameField.text = flowItem.modelData.name;
                            nameField.selectAll();
                            nameField.forceActiveFocus();
                        }
                    }

                    Text {
                        text: qsTr("P%1").arg(flowItem.modelData.preset + 1)
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeBase
                        Layout.rightMargin: Theme.spacingS
                    }

                    CButton {
                        symbol: flowItem.isRunning ? qsTr("Stop") : qsTr("Run")
                        Layout.preferredWidth: 60
                        enabled: flowItem.isRunning || !flowManager.running
                        onClicked: flowItem.isRunning ? flowManager.stopFlow() : flowManager.runFlow(flowItem.modelData.id)
                    }

                    CButton {
                        iconName: "menu"
                        enabled: !flowItem.isRunning
                        onClicked: flowMenu.popup()

                        CMenu {
                            id: flowMenu

                            CMenuItem {
                                text: qsTr("Edit")
                                onTriggered: {
                                    flowEditor.load(flowItem.modelData.id);
                                    flowEditor.open();
                                }
                            }
                            CMenuItem {
                                text: qsTr("Delete")
                                onTriggered: {
                                    deleteFlowDialog.pendingId = flowItem.modelData.id;
                                    deleteFlowDialog.pendingName = flowItem.modelData.name;
                                    deleteFlowDialog.open();
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    ConfirmDialog {
        id: deleteFlowDialog
        title: qsTr("Delete flow")
        message: qsTr("Delete the flow “%1”? This cannot be undone.").arg(deleteFlowDialog.pendingName)
        onAccepted: flowManager.removeFlow(deleteFlowDialog.pendingId)

        property string pendingId: ""
        property string pendingName: ""
    }
}
