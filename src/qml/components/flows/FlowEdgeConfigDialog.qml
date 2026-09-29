import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: edgeConfig
    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    padding: Theme.spacingL
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string edgeId: ""
    property bool fromStart: false
    property string triggerType: "immediate"
    property int triggerSeconds: 0
    property string originAction: "none"
    property string targetAction: "play"

    signal edited(string id, var patch)
    signal deleted(string id)

    function loadEdge(id, fromStartFlag, type, seconds, origin, target) {
        edgeConfig.edgeId = id;
        edgeConfig.fromStart = fromStartFlag;
        edgeConfig.triggerType = type;
        edgeConfig.triggerSeconds = seconds;
        edgeConfig.originAction = origin;
        edgeConfig.targetAction = target;
        edgeConfig.open();
    }

    function setTrigger(type) {
        edgeConfig.triggerType = type;
        edgeConfig.edited(edgeConfig.edgeId, {
            "triggerType": type
        });
    }

    function setSeconds(value) {
        const v = Math.max(0, value);
        edgeConfig.triggerSeconds = v;
        edgeConfig.edited(edgeConfig.edgeId, {
            "triggerSeconds": v
        });
    }

    function setOriginAction(action) {
        edgeConfig.originAction = action;
        edgeConfig.edited(edgeConfig.edgeId, {
            "originAction": action
        });
    }

    function setTargetAction(action) {
        edgeConfig.targetAction = action;
        edgeConfig.edited(edgeConfig.edgeId, {
            "targetAction": action
        });
    }

    background: Rectangle {
        color: Theme.bgPanel
        border.color: Theme.borderBright
        border.width: 2
        radius: Theme.radiusXS
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingL

        Text {
            text: qsTr("Link settings")
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMid
            font.bold: true
        }

        Text {
            text: qsTr("Trigger")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
            font.bold: true
        }

        RowLayout {
            spacing: Theme.spacingS

            CButton {
                symbol: qsTr("Immediate")
                highlighted: edgeConfig.triggerType === "immediate"
                onClicked: edgeConfig.setTrigger("immediate")
            }
            CButton {
                symbol: qsTr("After time")
                highlighted: edgeConfig.triggerType === "elapsed"
                onClicked: edgeConfig.setTrigger("elapsed")
            }
            CButton {
                symbol: qsTr("On finish")
                visible: !edgeConfig.fromStart
                highlighted: edgeConfig.triggerType === "finish"
                onClicked: edgeConfig.setTrigger("finish")
            }
        }

        RowLayout {
            spacing: Theme.spacingM
            visible: edgeConfig.triggerType === "elapsed"

            Text {
                text: qsTr("After")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
            }

            Item {
                Layout.preferredWidth: 50
                Layout.preferredHeight: 26

                CTextField {
                    id: secField
                    text: "" + edgeConfig.triggerSeconds
                    validator: IntValidator {
                        bottom: 0
                        top: 86400
                    }
                    inputMethodHints: Qt.ImhDigitsOnly
                    onEditingFinished: edgeConfig.setSeconds(parseInt(text) || 0)
                }
            }

            Text {
                text: qsTr("seconds after the source starts.")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
            }
        }

        Text {
            visible: !edgeConfig.fromStart
            text: qsTr("Action on origin")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
            font.bold: true
        }

        RowLayout {
            visible: !edgeConfig.fromStart
            spacing: Theme.spacingS

            CButton {
                symbol: qsTr("None")
                highlighted: edgeConfig.originAction === "none"
                onClicked: edgeConfig.setOriginAction("none")
            }
            CButton {
                symbol: qsTr("Fade out")
                highlighted: edgeConfig.originAction === "fadeOut"
                onClicked: edgeConfig.setOriginAction("fadeOut")
            }
            CButton {
                symbol: qsTr("Stop")
                highlighted: edgeConfig.originAction === "stop"
                onClicked: edgeConfig.setOriginAction("stop")
            }
        }

        Text {
            text: qsTr("Action on target")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
            font.bold: true
        }

        RowLayout {
            spacing: Theme.spacingS

            CButton {
                symbol: qsTr("Play")
                highlighted: edgeConfig.targetAction === "play"
                onClicked: edgeConfig.setTargetAction("play")
            }
            CButton {
                symbol: qsTr("Fade in")
                highlighted: edgeConfig.targetAction === "fadeIn"
                onClicked: edgeConfig.setTargetAction("fadeIn")
            }
        }

        RowLayout {
            Layout.topMargin: Theme.spacingM
            Layout.fillWidth: true
            spacing: Theme.spacingS

            CButton {
                symbol: qsTr("Delete")
                danger: true
                onClicked: {
                    edgeConfig.deleted(edgeConfig.edgeId);
                    edgeConfig.close();
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }
    }
}
