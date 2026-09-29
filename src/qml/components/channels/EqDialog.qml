import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: eqDialog
    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    padding: Theme.spacingL
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property ChannelEffects effects: null
    property string channelName: ""

    readonly property int sliderHeight: 200

    background: Rectangle {
        color: Theme.bgPanel
        border.color: Theme.borderBright
        border.width: 2
        radius: Theme.radiusXS
    }

    contentItem: ColumnLayout {
        spacing: Theme.spacingL

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingS

            Text {
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                text: qsTr("Equalizer — %1").arg(eqDialog.channelName)
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMid
                font.bold: true
                elide: Text.ElideRight
            }

            CButton {
                symbol: eqDialog.effects && eqDialog.effects.eqEnabled ? "ON" : "OFF"
                checkable: true
                checked: eqDialog.effects && eqDialog.effects.eqEnabled
                onToggled: {
                    if (eqDialog.effects)
                        eqDialog.effects.eqEnabled = checked;
                }
            }
            CButton {
                iconName: "reset"
                onClicked: {
                    if (eqDialog.effects)
                        eqDialog.effects.resetEq();
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 0
            Repeater {
                model: eqDialog.effects ? eqDialog.effects.eqBandLabels : []
                delegate: ParamSlider {
                    required property int index
                    required property string modelData

                    label: modelData
                    from: eqDialog.effects ? -eqDialog.effects.eqGainMax : -12
                    to: eqDialog.effects ? eqDialog.effects.eqGainMax : 12
                    value: eqDialog.effects ? eqDialog.effects.eqBands[index] : 0
                    onMoved: v => eqDialog.effects.setEqBandGain(index, v)
                }
            }
        }
    }

    component ParamSlider: ColumnLayout {
        id: ps
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: 44
        spacing: Theme.spacingXS

        property string label: ""
        property real from: -12
        property real to: 12
        property real step: 1
        property int decimals: 0
        property real value: 0
        signal moved(real value)

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: ps.value.toFixed(ps.decimals) + " dB"
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }

        VSlider {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: eqDialog.sliderHeight
            showMarks: false
            from: ps.from
            to: ps.to
            stepSize: ps.step
            value: ps.value
            onMoved: ps.moved(value)
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 44
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: ps.label
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }
    }
}
