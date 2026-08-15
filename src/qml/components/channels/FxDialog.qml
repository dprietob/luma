import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: fxDialog
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

        Text {
            Layout.fillWidth: true
            Layout.preferredWidth: 0
            text: qsTr("Effects — %1").arg(fxDialog.channelName)
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMid
            font.bold: true
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.spacingM

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                spacing: Theme.spacingL
                EffectHead {
                    title: qsTr("Reverb")
                    active: fxDialog.effects && fxDialog.effects.reverbEnabled
                    onToggled: on => fxDialog.effects.reverbEnabled = on
                    onResetRequested: fxDialog.effects.resetReverb()
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 0
                    ParamSlider {
                        label: qsTr("Room")
                        value: fxDialog.effects ? fxDialog.effects.reverbRoomSize : 0
                        onMoved: v => fxDialog.effects.reverbRoomSize = v
                    }
                    ParamSlider {
                        label: qsTr("Damping")
                        value: fxDialog.effects ? fxDialog.effects.reverbDamping : 0
                        onMoved: v => fxDialog.effects.reverbDamping = v
                    }
                    ParamSlider {
                        label: qsTr("Mix")
                        value: fxDialog.effects ? fxDialog.effects.reverbMix : 0
                        onMoved: v => fxDialog.effects.reverbMix = v
                    }
                }
            }

            VSeparator {}

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                spacing: Theme.spacingL
                EffectHead {
                    title: qsTr("Delay")
                    active: fxDialog.effects && fxDialog.effects.delayEnabled
                    onToggled: on => fxDialog.effects.delayEnabled = on
                    onResetRequested: fxDialog.effects.resetDelay()
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 0
                    ParamSlider {
                        label: qsTr("Time")
                        from: 0
                        to: 1000
                        step: 1
                        decimals: 0
                        suffix: " ms"
                        value: fxDialog.effects ? fxDialog.effects.delayTime : 0
                        onMoved: v => fxDialog.effects.delayTime = v
                    }
                    ParamSlider {
                        label: qsTr("Feedback")
                        from: 0
                        to: 0.95
                        value: fxDialog.effects ? fxDialog.effects.delayFeedback : 0
                        onMoved: v => fxDialog.effects.delayFeedback = v
                    }
                    ParamSlider {
                        label: qsTr("Mix")
                        value: fxDialog.effects ? fxDialog.effects.delayMix : 0
                        onMoved: v => fxDialog.effects.delayMix = v
                    }
                }
            }

            VSeparator {}

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                spacing: Theme.spacingL
                EffectHead {
                    title: qsTr("Distortion")
                    active: fxDialog.effects && fxDialog.effects.distortionEnabled
                    onToggled: on => fxDialog.effects.distortionEnabled = on
                    onResetRequested: fxDialog.effects.resetDistortion()
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 0
                    ParamSlider {
                        label: qsTr("Drive")
                        from: 1
                        to: 50
                        step: 0.1
                        decimals: 1
                        value: fxDialog.effects ? fxDialog.effects.distortionDrive : 1
                        onMoved: v => fxDialog.effects.distortionDrive = v
                    }
                    ParamSlider {
                        label: qsTr("Mix")
                        value: fxDialog.effects ? fxDialog.effects.distortionMix : 0
                        onMoved: v => fxDialog.effects.distortionMix = v
                    }
                }
            }

            VSeparator {}

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                spacing: Theme.spacingL
                EffectHead {
                    title: qsTr("Pitch")
                    active: fxDialog.effects && fxDialog.effects.pitchEnabled
                    onToggled: on => fxDialog.effects.pitchEnabled = on
                    onResetRequested: fxDialog.effects.resetPitch()
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 0
                    ParamSlider {
                        label: qsTr("Semitones")
                        from: -12
                        to: 12
                        step: 1
                        decimals: 0
                        suffix: " st"
                        value: fxDialog.effects ? fxDialog.effects.pitchSemitones : 0
                        onMoved: v => fxDialog.effects.pitchSemitones = v
                    }
                }
            }

            VSeparator {}

            ColumnLayout {
                Layout.alignment: Qt.AlignTop
                spacing: Theme.spacingL
                EffectHead {
                    title: qsTr("Speed")
                    active: fxDialog.effects && fxDialog.effects.speedEnabled
                    onToggled: on => fxDialog.effects.speedEnabled = on
                    onResetRequested: fxDialog.effects.resetSpeed()
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 0
                    ParamSlider {
                        label: qsTr("Tempo")
                        from: 0.5
                        to: 2.0
                        step: 0.01
                        suffix: "x"
                        value: fxDialog.effects ? fxDialog.effects.speedTempo : 1
                        onMoved: v => fxDialog.effects.speedTempo = v
                    }
                }
            }
        }
    }

    component EffectHead: ColumnLayout {
        id: head
        Layout.alignment: Qt.AlignHCenter
        spacing: Theme.spacingM

        property string title: ""
        property bool active: false
        signal toggled(bool on)
        signal resetRequested

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: head.title
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
            font.bold: true
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.spacingS
            CButton {
                symbol: head.active ? "ON" : "OFF"
                checkable: true
                checked: head.active
                onToggled: head.toggled(checked)
            }
            CButton {
                iconName: "reset"
                onClicked: head.resetRequested()
            }
        }
    }

    component ParamSlider: ColumnLayout {
        id: ps
        Layout.alignment: Qt.AlignTop
        Layout.preferredWidth: 40
        spacing: Theme.spacingXS

        property string label: ""
        property real from: 0
        property real to: 1
        property real step: 0.01
        property int decimals: 2
        property string suffix: ""
        property real value: 0
        signal moved(real value)

        Text {
            Layout.alignment: Qt.AlignHCenter
            text: ps.value.toFixed(ps.decimals) + ps.suffix
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
            elide: Text.ElideRight
        }

        VSlider {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredHeight: fxDialog.sliderHeight
            showMarks: false
            from: ps.from
            to: ps.to
            stepSize: ps.step
            value: ps.value
            onMoved: ps.moved(value)
        }

        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 46
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: ps.label
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }
    }
}
