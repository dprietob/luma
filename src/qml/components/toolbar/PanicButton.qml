import QtQuick

CButton {
    id: panicButton
    symbol: qsTr("PANIC")
    danger: true
    onClicked: audioEngine.panic()
}
