import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: nodeConfig
    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    padding: Theme.spacingL
    closePolicy: Popup.CloseOnEscape
    width: 500

    property string nodeId: ""
    property string nodeLabel: ""

    signal applied(string id)

    function pad2(n) {
        return n < 10 ? "0" + n : "" + n;
    }

    function loadNode(id, label) {
        nodeConfig.nodeId = id;
        nodeConfig.nodeLabel = label;
        minField.text = "" + Math.floor(flowNodeConfig.startSeconds / 60);
        secField.text = nodeConfig.pad2(Math.floor(flowNodeConfig.startSeconds % 60));
        nodeConfig.open();
    }

    function commitStart() {
        const m = parseInt(minField.text) || 0;
        const s = parseInt(secField.text) || 0;
        flowNodeConfig.startSeconds = m * 60 + s;
    }

    background: Rectangle {
        color: Theme.bgPanel
        border.color: Theme.borderBright
        border.width: 2
        radius: Theme.radiusXS
    }

    component ParamSlider: RowLayout {
        id: rootRow
        property string label: ""
        property real from: 0
        property real to: 1
        property real value: 0
        property string valueText: ""
        signal moved(real v)
        spacing: Theme.spacingM

        Text {
            Layout.preferredWidth: 110
            text: rootRow.label
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        HSlider {
            Layout.fillWidth: true
            from: rootRow.from
            to: rootRow.to
            value: rootRow.value
            onMoved: rootRow.moved(value)
        }
        Text {
            Layout.preferredWidth: 54
            text: rootRow.valueText
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
            horizontalAlignment: Text.AlignRight
        }
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingM

        Text {
            text: qsTr("Node — %1").arg(nodeConfig.nodeLabel)
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMid
            font.bold: true
        }

        ParamSlider {
            Layout.fillWidth: true
            label: qsTr("Balance")
            from: -1
            to: 1
            value: flowNodeConfig.pan
            valueText: flowNodeConfig.pan.toFixed(2)
            onMoved: v => flowNodeConfig.pan = v
        }

        ParamSlider {
            Layout.fillWidth: true
            label: qsTr("Initial volume")
            value: flowNodeConfig.initialVolume
            valueText: Math.round(flowNodeConfig.initialVolume * 100) + "%"
            onMoved: v => flowNodeConfig.initialVolume = v
        }

        ParamSlider {
            Layout.fillWidth: true
            label: qsTr("Fade-in max")
            value: flowNodeConfig.fadeMax
            valueText: Math.round(flowNodeConfig.fadeMax * 100) + "%"
            onMoved: v => flowNodeConfig.fadeMax = v
        }

        ParamSlider {
            Layout.fillWidth: true
            label: qsTr("Fade-out min")
            value: flowNodeConfig.fadeMin
            valueText: Math.round(flowNodeConfig.fadeMin * 100) + "%"
            onMoved: v => flowNodeConfig.fadeMin = v
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingM
            spacing: Theme.spacingM

            Text {
                Layout.preferredWidth: 110
                text: qsTr("Fade duration")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
                elide: Text.ElideRight
            }

            Item {
                Layout.preferredWidth: 50
                Layout.preferredHeight: 26

                CTextField {
                    id: fadeSecField
                    text: "" + flowNodeConfig.fadeSeconds
                    validator: IntValidator {
                        bottom: 0
                        top: 3600
                    }
                    inputMethodHints: Qt.ImhDigitsOnly
                    onEditingFinished: flowNodeConfig.fadeSeconds = parseInt(text) || 0
                }
            }

            Text {
                text: qsTr("seconds")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
            }

            Item {
                Layout.fillWidth: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingM
            spacing: Theme.spacingM

            Text {
                Layout.preferredWidth: 110
                text: qsTr("Fade mode")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
                elide: Text.ElideRight
            }

            CButton {
                iconName: "left"
                Layout.preferredWidth: 30
                enabled: flowNodeConfig.fadeMode > 0
                onClicked: flowNodeConfig.fadeMode = flowNodeConfig.fadeMode - 1
            }
            Text {
                Layout.preferredWidth: 110
                text: FadeModes.labelAt(flowNodeConfig.fadeMode)
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
                horizontalAlignment: Text.AlignHCenter
            }
            CButton {
                iconName: "right"
                Layout.preferredWidth: 30
                enabled: flowNodeConfig.fadeMode < FadeModes.count() - 1
                onClicked: flowNodeConfig.fadeMode = flowNodeConfig.fadeMode + 1
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingM
            spacing: Theme.spacingM

            Text {
                Layout.preferredWidth: 110
                text: qsTr("Start position")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
                elide: Text.ElideRight
            }

            Item {
                Layout.preferredWidth: 46
                Layout.preferredHeight: 26

                CTextField {
                    id: minField
                    validator: IntValidator {
                        bottom: 0
                        top: 999
                    }
                    inputMethodHints: Qt.ImhDigitsOnly
                    onEditingFinished: nodeConfig.commitStart()
                }
            }

            Text {
                text: ":"
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
            }

            Item {
                Layout.preferredWidth: 46
                Layout.preferredHeight: 26

                CTextField {
                    id: secField
                    validator: IntValidator {
                        bottom: 0
                        top: 59
                    }
                    inputMethodHints: Qt.ImhDigitsOnly
                    onEditingFinished: nodeConfig.commitStart()
                }
            }

            Text {
                text: qsTr("min:sec")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
            }

            Item {
                Layout.fillWidth: true
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.spacingL
            spacing: Theme.spacingS

            CButton {
                iconName: "effects"
                highlighted: flowNodeConfig.effects.anyEnabled
                onClicked: fxDialog.open()
            }
            CButton {
                iconName: "equalizer"
                highlighted: flowNodeConfig.effects.eqEnabled
                onClicked: eqDialog.open()
            }
            CButton {
                iconName: "repeat"
                checkable: true
                checked: flowNodeConfig.loop
                onToggled: flowNodeConfig.loop = checked
            }

            Item {
                Layout.fillWidth: true
            }

            CButton {
                symbol: qsTr("Save")
                onClicked: {
                    nodeConfig.applied(nodeConfig.nodeId);
                    nodeConfig.close();
                }
            }
            CButton {
                symbol: qsTr("Cancel")
                onClicked: nodeConfig.close()
            }
        }
    }

    FxDialog {
        id: fxDialog
        effects: flowNodeConfig.effects
        channelName: nodeConfig.nodeLabel
    }

    EqDialog {
        id: eqDialog
        effects: flowNodeConfig.effects
        channelName: nodeConfig.nodeLabel
    }

    onClosed: {
        fxDialog.close();
        eqDialog.close();
    }
}
