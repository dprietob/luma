import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: channelCountDialog
    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    padding: Theme.spacingL
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    onOpened: channelCountDialog.pending = audioEngine.channelCount

    property int pending: 12

    function clampPending(v) {
        channelCountDialog.pending = Math.max(1, Math.min(40, v));
    }

    function apply() {
        const n = channelCountDialog.pending;
        if (n === audioEngine.channelCount) {
            channelCountDialog.close();
            return;
        }
        if (n < audioEngine.channelCount) {
            confirmReduce.open();
        } else {
            audioEngine.channelCount = n;
            channelCountDialog.close();
        }
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
            text: qsTr("Number of channels")
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMid
            font.bold: true
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.spacingS

            CButton {
                symbol: "−"
                Layout.preferredHeight: 30
                onClicked: channelCountDialog.clampPending(channelCountDialog.pending - 1)
            }

            TextField {
                id: field
                Layout.preferredWidth: 70
                Layout.preferredHeight: 30
                horizontalAlignment: TextInput.AlignHCenter
                verticalAlignment: TextInput.AlignVCenter
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMid
                text: channelCountDialog.pending
                inputMethodHints: Qt.ImhDigitsOnly
                validator: IntValidator {
                    bottom: 1
                    top: 40
                }
                onTextEdited: {
                    const v = parseInt(text, 10);
                    if (!isNaN(v))
                        channelCountDialog.clampPending(v);
                }

                background: Rectangle {
                    color: Theme.bgControl
                    border.color: field.activeFocus ? Theme.accent : Theme.borderSubtle
                    border.width: 1
                    radius: Theme.radiusXS
                }
            }

            CButton {
                symbol: "+"
                Layout.preferredHeight: 30
                onClicked: channelCountDialog.clampPending(channelCountDialog.pending + 1)
            }
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Between 1 and 40 · applies to every preset")
            color: Theme.textMuted
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingM

            CButton {
                symbol: qsTr("Apply")
                Layout.preferredWidth: 80
                onClicked: channelCountDialog.apply()
            }

            CButton {
                symbol: qsTr("Cancel")
                Layout.preferredWidth: 80
                onClicked: channelCountDialog.close()
            }
        }
    }

    ConfirmDialog {
        id: confirmReduce
        title: qsTr("Reduce channels")
        message: qsTr("The last %1 channel(s) will be removed and their configuration and track binding will be lost. Continue?").arg(audioEngine.channelCount - channelCountDialog.pending)
        onAccepted: {
            audioEngine.channelCount = channelCountDialog.pending;
            channelCountDialog.close();
        }
    }
}
