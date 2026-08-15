import QtQml
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: presetsArea
    color: Theme.bgBase

    implicitWidth: buttonsRow.implicitWidth
    implicitHeight: buttonsRow.implicitHeight

    RowLayout {
        id: buttonsRow
        anchors.fill: parent
        spacing: Theme.spacingS

        Repeater {
            model: presetManager.count
            delegate: PresetButton {
                index: modelData
                onActivationRequested: i => {
                    confirmDialog.targetPreset = i;
                    confirmDialog.open();
                }
            }
        }

        Item {
            Layout.preferredWidth: Theme.spacingXL
        }
    }

    Instantiator {
        model: 10
        delegate: Shortcut {
            required property int index
            sequence: (appSettings.shortcutsRevision, appSettings.shortcut("preset" + (index + 1)))
            enabled: !appSettings.capturing
            onActivated: {
                if (index >= presetManager.count)
                    return;
                confirmDialog.targetPreset = index;
                confirmDialog.open();
            }
        }
    }

    PresetDialog {
        id: confirmDialog
    }
}
