import QtQuick
import QtQuick.Controls
import QtTest

TestCase {
    id: tc
    name: "FaderRealChain"
    width: 100; height: 300
    visible: true
    when: windowShown

    Slider {
        id: s
        anchors.fill: parent
        orientation: Qt.Vertical
        from: 0; to: 1
        property var chan: audioEngine.channelAt(0)
        // Fader lineal: value = ganancia del canal directamente.
        value: chan.volume
        onMoved: audioEngine.setVolume(0, value)
    }

    function test_drag_reduces_real_volume() {
        verify(audioEngine.bindTrack(0, testFile), "bind ok")
        var before = s.chan.volume
        mousePress(s, 50, 15)
        mouseMove(s, 50, 150)
        mouseMove(s, 50, 285)
        mouseRelease(s, 50, 285)
        verify(s.chan.volume < before - 0.1,
               "real volume " + before.toFixed(3) + " -> " + s.chan.volume.toFixed(3))
    }
}
