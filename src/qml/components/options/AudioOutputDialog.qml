import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: audioOutputDialog
    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    width: 480
    padding: Theme.spacingL
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    onOpened: audioDevices.refresh()

    background: Rectangle {
        color: Theme.bgPanel
        border.color: Theme.borderBright
        border.width: 2
        radius: Theme.radiusXS
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingL

        Text {
            Layout.fillWidth: true
            text: qsTr("Audio output")
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMid
            font.bold: true
        }

        DeviceSelector {
            label: qsTr("Main")
            value: audioDevices.mainOutput
            onSelected: name => audioDevices.mainOutput = name
        }

        DeviceSelector {
            label: qsTr("Aux")
            value: audioDevices.auxOutput
            onSelected: name => audioDevices.auxOutput = name
        }
    }

    component DeviceSelector: RowLayout {
        id: selector
        Layout.fillWidth: true
        spacing: Theme.spacingM

        property string label: ""
        property string value: ""
        signal selected(string name)

        Text {
            Layout.preferredWidth: 60
            text: selector.label
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
        }

        CButton {
            Layout.fillWidth: true
            symbol: selector.value.length > 0 ? selector.value : qsTr("System default")
            onClicked: deviceMenu.open()

            CMenu {
                id: deviceMenu
                width: 350
                CMenuItem {
                    text: qsTr("System default")
                    onTriggered: selector.selected("")
                }
                Repeater {
                    model: audioDevices.outputDevices
                    delegate: CMenuItem {
                        required property var modelData
                        text: modelData.name
                        onTriggered: selector.selected(modelData.name)
                    }
                }
            }
        }
    }
}
