import QtQuick
import QtQuick.Layouts

RowLayout {
    id: fader
    spacing: Theme.spacingL

    property int size: -1
    property int vuWidth: 180

    property alias value: slider.value
    property alias vuLeft: vu.levelLeft
    property alias vuRight: vu.levelRight

    signal moved(real value)
    signal resetRequested

    Layout.fillHeight: true
    Layout.fillWidth: (fader.size === -1)
    Layout.preferredWidth: fader.size

    HSlider {
        id: slider
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
        onMoved: fader.moved(slider.value)
        onResetRequested: fader.resetRequested()
    }

    HVuMeter {
        id: vu
        Layout.preferredWidth: fader.vuWidth
        Layout.preferredHeight: 18
        Layout.alignment: Qt.AlignVCenter
    }
}
