import QtQuick
import QtQuick.Dialogs
import QtQuick.Layouts

Rectangle {
    id: strip
    color: Theme.bgSubPanel
    border.color: Theme.borderSubtle
    border.width: 1
    implicitWidth: content.implicitWidth + 2 * (Theme.spacingS + border.width)
    onFadeLevelChanged: if (volumeFade.running)
        audioEngine.setVolume(strip.channel.id, fadeLevel)
    opacity: strip.channel.hasTrack ? 1 : 0.3

    required property var channel
    property real fadeLevel: 0
    readonly property bool flowActive: flowManager.activeChannels.indexOf(strip.channel.id) >= 0

    SequentialAnimation {
        running: strip.flowActive
        loops: Animation.Infinite
        onStopped: strip.border.color = Theme.borderSubtle
        ColorAnimation {
            target: strip
            property: "border.color"
            from: Theme.borderSubtle
            to: Theme.accent
            duration: 500
        }
        ColorAnimation {
            target: strip
            property: "border.color"
            from: Theme.accent
            to: Theme.borderSubtle
            duration: 500
        }
    }

    NumberAnimation {
        id: volumeFade
        target: strip
        property: "fadeLevel"
        easing.type: Easing.Linear
        onFinished: if (volumeFade.to === 0.0)
            audioEngine.pauseChannel(strip.channel.id)
    }

    Connections {
        target: presetManager
        function onLoadingChanged() {
            if (presetManager.loading)
                volumeFade.stop();
        }
    }

    function rampVolumeTo(target) {
        volumeFade.stop();
        const mode = FadeModes.modes[strip.channel.fadeMode] || FadeModes.modes[0];
        volumeFade.easing.type = mode.easingType;
        if (mode.bezier !== undefined)
            volumeFade.easing.bezierCurve = mode.bezier;
        volumeFade.from = strip.channel.volume;
        volumeFade.to = target;
        volumeFade.duration = Math.max(1, strip.channel.fadeSeconds * 1000);
        volumeFade.start();
    }

    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: contextMenu.popup()
    }

    ColumnLayout {
        id: content
        anchors.fill: parent
        anchors.margins: Theme.spacingS
        spacing: Theme.spacingS

        Rectangle {
            id: group
            color: strip.channel.color
            Layout.fillWidth: true
            Layout.bottomMargin: Theme.spacingS
            height: 4

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: colorDialog.open()
            }
        }

        RowLayout {
            spacing: Theme.spacingL

            Item {
                id: nameBox
                Layout.preferredHeight: 25
                Layout.fillWidth: true
                property bool editing: false

                Text {
                    anchors.fill: parent
                    visible: !nameBox.editing
                    text: strip.channel.name
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBase
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight

                    TapHandler {
                        onDoubleTapped: nameBox.editing = true
                    }
                }

                CTextField {
                    id: nameField
                    visible: nameBox.editing
                    onAccepted: commit()
                    onActiveFocusChanged: if (!activeFocus && nameBox.editing)
                        commit()

                    function commit() {
                        const trimmed = text.trim();
                        if (trimmed.length > 0)
                            strip.channel.name = trimmed;
                        nameBox.editing = false;
                    }
                }

                onEditingChanged: {
                    if (!editing)
                        return;
                    nameField.text = strip.channel.name;
                    nameField.selectAll();
                    nameField.forceActiveFocus();
                }
            }

            ChannelIndicator {
                hasTrack: strip.channel.hasTrack
                trackName: strip.channel.filePath
            }
        }

        PanDial {
            value: strip.channel.pan
            onMoved: audioEngine.setPan(strip.channel.id, value)
            onResetRequested: audioEngine.setPan(strip.channel.id, 0.0)
        }

        HSeparator {}

        VFader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.topMargin: Theme.spacingM
            Layout.bottomMargin: Theme.spacingM
            value: strip.channel.volume
            vuLeft: strip.channel.vuLeft
            vuRight: strip.channel.vuRight
            rangeHandles: true
            rangeMax: strip.channel.fadeMax
            rangeMin: strip.channel.fadeMin
            onMoved: value => {
                volumeFade.stop();
                audioEngine.setVolume(strip.channel.id, value);
            }
            onResetRequested: {
                volumeFade.stop();
                audioEngine.setVolume(strip.channel.id, 1.0);
            }
            onRangeMaxMoved: value => strip.channel.fadeMax = value
            onRangeMinMoved: value => strip.channel.fadeMin = value
        }

        HSeparator {}

        FadeButtons {
            isFadingIn: volumeFade.running && volumeFade.to > volumeFade.from
            isFadingOut: volumeFade.running && volumeFade.to < volumeFade.from
            fadeSeconds: strip.channel.fadeSeconds
            fadeMode: strip.channel.fadeMode
            onFadeInRequested: {
                if (volumeFade.running && volumeFade.to > volumeFade.from) {
                    volumeFade.stop();
                    return;
                }
                if (!strip.channel.isPlaying)
                    audioEngine.playChannel(strip.channel.id);
                strip.rampVolumeTo(strip.channel.fadeMax);
            }
            onFadeOutRequested: {
                if (volumeFade.running && volumeFade.to < volumeFade.from) {
                    volumeFade.stop();
                    return;
                }
                strip.rampVolumeTo(strip.channel.fadeMin);
            }
            onFadeSecondsEdited: seconds => strip.channel.fadeSeconds = seconds
            onFadeModeSelected: mode => strip.channel.fadeMode = mode
        }

        HSeparator {}

        CButton {
            id: auxButton
            symbol: "AUX"
            fontSmall: true
            fullWidth: true
            checkable: true
            checked: strip.channel.aux
            onToggled: strip.channel.aux = checked
            Layout.leftMargin: Theme.spacingS
            Layout.rightMargin: Theme.spacingS
        }

        EffectButtons {
            fxActive: strip.channel.effects.anyEnabled
            eqActive: strip.channel.effects.eqEnabled
            onFxRequested: fxDialog.open()
            onEqRequested: eqDialog.open()
        }

        CButton {
            id: repeatButton
            iconName: "repeat"
            fontSmall: true
            fullWidth: true
            checkable: true
            checked: strip.channel.loop
            onToggled: strip.channel.loop = checked
            Layout.leftMargin: Theme.spacingS
            Layout.rightMargin: Theme.spacingS
        }

        HSeparator {}

        TransportButtons {
            isPlaying: strip.channel.isPlaying
            isPaused: strip.channel.isPaused
            onPlayPauseRequested: strip.channel.isPlaying ? audioEngine.pauseChannel(strip.channel.id) : audioEngine.playChannel(strip.channel.id)
            onStopRequested: audioEngine.stopChannel(strip.channel.id)
        }

        ChannelProgress {
            channel: strip.channel
            onClicked: progressDialog.open()
        }
    }

    DropArea {
        id: trackDrop
        anchors.fill: parent
        keys: ["trackFile"]
        onDropped: drop => {
            if (drop.source && drop.source.filePath !== undefined)
                audioEngine.bindTrack(strip.channel.id, drop.source.filePath);
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: Theme.accent
        border.width: 2
        visible: trackDrop.containsDrag
    }

    ColorDialog {
        id: colorDialog
        title: qsTr("Channel %1 color").arg(strip.channel.name)
        selectedColor: strip.channel.color
        onAccepted: strip.channel.color = colorDialog.selectedColor
    }

    FxDialog {
        id: fxDialog
        effects: strip.channel.effects
        channelName: strip.channel.name
    }

    EqDialog {
        id: eqDialog
        effects: strip.channel.effects
        channelName: strip.channel.name
    }

    ProgressDialog {
        id: progressDialog
        channel: strip.channel
        channelName: strip.channel.name
    }

    FileDialog {
        id: trackDialog
        title: qsTr("Bind file to channel %1").arg(strip.channel.name)
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Audio (*.wav *.mp3 *.ogg *.flac)"), qsTr("All files (*)")]
        onAccepted: audioEngine.bindTrackFromUrl(strip.channel.id, selectedFile)
    }

    CMenu {
        id: contextMenu
        CMenuItem {
            text: qsTr("Bind track…")
            onTriggered: trackDialog.open()
        }
        CMenuItem {
            text: qsTr("Unbind channel")
            enabled: strip.channel.hasTrack
            onTriggered: audioEngine.unbindChannel(strip.channel.id)
        }
        HSeparator {}
        CMenuItem {
            text: qsTr("Set color…")
            onTriggered: colorDialog.open()
        }
        CMenuItem {
            text: qsTr("Reset color")
            onTriggered: audioEngine.resetChannelColor(strip.channel.id)
        }
    }
}
