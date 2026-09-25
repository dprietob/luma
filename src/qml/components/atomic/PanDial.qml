import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Dial {
    id: pan
    from: -1.0
    to: 1.0
    stepSize: 0.01
    inputMode: Dial.Horizontal
    implicitWidth: 47
    implicitHeight: 47
    Layout.fillWidth: true

    signal resetRequested

    readonly property bool centered: Math.round(value * 100) === 0

    readonly property string valueText: {
        const v = Math.round(value * 100);
        return v === 0 ? "C" : (v < 0 ? "L" + (-v) : "R" + v);
    }

    property double lastPressMs: -1
    onPressedChanged: {
        if (!pressed)
            return;
        const now = Date.now();
        if (lastPressMs > 0 && now - lastPressMs < 350)
            resetRequested();
        lastPressMs = now;
    }

    background: Item {
        anchors.fill: parent

        readonly property real diameter: Math.min(parent.width, parent.height - labelRow.height - 2)

        Rectangle {
            id: knob
            width: parent.diameter
            height: parent.diameter
            radius: width / 2
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            color: Theme.bgControl
            border.color: pan.centered ? Theme.borderSubtle : Theme.accent
            border.width: 1

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                y: 2
                width: 2
                height: 4
                radius: 1
                color: Theme.textSecondary
            }

            Rectangle {
                width: 2
                height: knob.height / 2 - 3
                radius: 1
                x: knob.width / 2 - width / 2
                y: 3
                color: pan.centered ? Theme.textSecondary : Theme.accent
                transformOrigin: Item.Bottom
                rotation: pan.angle
            }
        }

        Item {
            id: labelRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: valueLabel.implicitHeight

            Text {
                id: valueLabel
                anchors.horizontalCenter: parent.horizontalCenter
                text: pan.valueText
                color: pan.centered ? Theme.textMuted : Theme.accent
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeSmall
            }
        }
    }

    handle: Item {}
}
