import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: progressDialog
    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    padding: Theme.spacingL
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var channel: null
    property string channelName: ""

    readonly property TrackTimeline tl: channel ? channel.timeline : null
    readonly property real position: channel ? channel.progress : 0
    readonly property real effStart: tl ? (tl.hasStart ? tl.start : 0) : 0
    readonly property real effEnd: tl ? (tl.hasEnd ? tl.end : 1) : 1

    onOpened: waveCanvas.requestPaint()

    function formatTime(seconds) {
        if (!seconds || seconds <= 0)
            return "0:00";
        const m = Math.floor(seconds / 60);
        const s = Math.floor(seconds % 60);
        return m + ":" + (s < 10 ? "0" : "") + s;
    }

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
            text: qsTr("Track — %1").arg(progressDialog.channelName)
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMid
            font.bold: true
            elide: Text.ElideRight
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingL

            Text {
                text: qsTr("Duration: %1").arg(progressDialog.formatTime(progressDialog.tl ? progressDialog.tl.durationSeconds : 0))
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
            }
            Text {
                text: qsTr("Position: %1").arg(progressDialog.formatTime((progressDialog.tl ? progressDialog.tl.durationSeconds : 0) * progressDialog.position))
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
            }
            Item {
                Layout.fillWidth: true
            }
            Text {
                visible: progressDialog.tl && (progressDialog.tl.hasStart || progressDialog.tl.hasEnd)
                text: qsTr("Region: %1").arg(progressDialog.formatTime(progressDialog.tl ? progressDialog.tl.regionSeconds : 0))
                color: Theme.accent
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
                font.bold: true
            }
        }

        Item {
            id: track
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 640
            Layout.preferredHeight: 160

            Rectangle {
                anchors.fill: parent
                color: Theme.bgSubPanel
                border.color: Theme.borderMid
                border.width: 1
                radius: Theme.radiusXS
            }

            Canvas {
                id: waveCanvas
                anchors.fill: parent
                anchors.margins: 1

                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()
                Component.onCompleted: requestPaint()

                onPaint: {
                    const ctx = getContext("2d");
                    ctx.reset();
                    const timeline = progressDialog.tl;
                    if (!timeline)
                        return;
                    const data = timeline.waveform;
                    const n = data.length;
                    if (n === 0)
                        return;
                    const w = width;
                    const mid = height / 2;
                    ctx.fillStyle = Theme.trackLoaded;
                    for (let i = 0; i < n; i++) {
                        const x = i * w / n;
                        const bw = Math.max(1, w / n);
                        const peak = data[i] * mid;
                        ctx.fillRect(x, mid - peak, bw, Math.max(1, peak * 2));
                    }
                }
            }

            Connections {
                target: progressDialog.tl
                function onWaveformChanged() {
                    waveCanvas.requestPaint();
                }
            }

            Rectangle {
                visible: progressDialog.tl && progressDialog.tl.regionEnabled
                x: 0
                y: 0
                height: track.height
                width: progressDialog.effStart * track.width
                color: Theme.bgBase
                opacity: 0.55
            }

            Rectangle {
                visible: progressDialog.tl && progressDialog.tl.regionEnabled
                y: 0
                height: track.height
                x: progressDialog.effEnd * track.width
                width: track.width - x
                color: Theme.bgBase
                opacity: 0.55
            }

            Rectangle {
                visible: progressDialog.channel && progressDialog.channel.hasTrack
                x: progressDialog.position * track.width - width / 2
                y: 0
                width: 2
                height: track.height
                color: Theme.textPrimary
            }

            Handle {
                frac: progressDialog.effStart
                active: progressDialog.tl && progressDialog.tl.hasStart
                onMoved: f => {
                    if (progressDialog.tl)
                        progressDialog.tl.setStart(f);
                }
            }

            Handle {
                frac: progressDialog.effEnd
                active: progressDialog.tl && progressDialog.tl.hasEnd
                onMoved: f => {
                    if (progressDialog.tl)
                        progressDialog.tl.setEnd(f);
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.spacingS

            CButton {
                symbol: progressDialog.tl && progressDialog.tl.regionEnabled ? "ON" : "OFF"
                checkable: true
                checked: progressDialog.tl && progressDialog.tl.regionEnabled
                Layout.preferredWidth: 70
                onToggled: {
                    if (progressDialog.tl)
                        progressDialog.tl.regionEnabled = checked;
                }
            }
            CButton {
                symbol: qsTr("Clear start")
                enabled: progressDialog.tl && progressDialog.tl.hasStart
                Layout.preferredWidth: 110
                onClicked: progressDialog.tl.clearStart()
            }
            CButton {
                symbol: qsTr("Clear end")
                enabled: progressDialog.tl && progressDialog.tl.hasEnd
                Layout.preferredWidth: 110
                onClicked: progressDialog.tl.clearEnd()
            }
            CButton {
                iconName: "reset"
                enabled: progressDialog.tl && (progressDialog.tl.hasStart || progressDialog.tl.hasEnd)
                Layout.preferredWidth: 90
                onClicked: progressDialog.tl.reset()
            }
        }
    }

    component Handle: Item {
        id: handle

        property real frac: 0
        property bool active: false
        signal moved(real f)

        width: 14
        height: track.height
        x: frac * track.width - width / 2

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 2
            height: parent.height
            color: handle.active ? Theme.accent : Theme.borderBright
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 0
            width: 10
            height: 10
            radius: Theme.radiusXS
            color: handle.active ? Theme.accent : Theme.borderBright
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeHorCursor
            onPositionChanged: mouse => {
                const pt = handle.mapToItem(track, mouse.x, mouse.y);
                handle.moved(pt.x / track.width);
            }
        }
    }
}
