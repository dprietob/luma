import QtQuick

CButton {
    id: presetButton

    property int index: 0
    readonly property bool active: presetManager.activePreset === index

    signal activationRequested(int index)

    symbol: qsTr("P%1").arg(presetButton.index + 1)
    fontSmall: true
    highlighted: presetButton.active
    enabled: !presetManager.loading && !flowManager.running
    onClicked: presetButton.activationRequested(index)
}
