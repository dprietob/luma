import QtQuick
import QtQuick.Layouts

RowLayout {
    id: fader
    spacing: Theme.spacingL

    property alias value: slider.value
    property alias vuLeft: vu.levelLeft
    property alias vuRight: vu.levelRight
    property alias rangeHandles: slider.rangeHandles
    property alias rangeMax: slider.rangeMax
    property alias rangeMin: slider.rangeMin

    signal moved(real value)
    signal resetRequested
    signal rangeMaxMoved(real value)
    signal rangeMinMoved(real value)

    VSlider {
        id: slider
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignHCenter
        onMoved: fader.moved(slider.value)
        onResetRequested: fader.resetRequested()
        onRangeMaxMoved: value => fader.rangeMaxMoved(value)
        onRangeMinMoved: value => fader.rangeMinMoved(value)
    }

    VVuMeter {
        id: vu
        Layout.fillHeight: true
        Layout.alignment: Qt.AlignHCenter
    }
}
