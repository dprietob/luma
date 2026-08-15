import QtQuick

ConfirmDialog {
    id: presetDialog
    title: qsTr("Change preset")
    message: qsTr("Switch to preset %1? Current playback will stop.").arg(presetDialog.targetPreset + 1)
    onAccepted: presetManager.selectPreset(presetDialog.targetPreset)

    property int targetPreset: -1
}
