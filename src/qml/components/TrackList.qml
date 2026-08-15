import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

Rectangle {
    id: trackListPanel
    color: Theme.bgPanel

    Rectangle {
        id: dragProxy
        parent: Overlay.overlay
        z: 9999
        visible: false
        width: 200
        height: 26
        radius: Theme.radiusXS
        color: Theme.bgHover
        border.color: Theme.accent
        border.width: 1
        opacity: 0.9

        property string filePath: ""

        Drag.active: false
        Drag.keys: ["trackFile"]
        Drag.hotSpot.x: width / 2
        Drag.hotSpot.y: height / 2

        Text {
            anchors.fill: parent
            anchors.margins: Theme.spacingS
            text: dragProxy.filePath.split("/").pop()
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
    }

    Shortcut {
        sequence: (appSettings.shortcutsRevision, appSettings.shortcut("addTrack"))
        enabled: !appSettings.capturing
        onActivated: importDialog.open()
    }

    FileDialog {
        id: importDialog
        title: qsTr("Add audio files")
        fileMode: FileDialog.OpenFiles
        nameFilters: [qsTr("Audio (*.wav *.mp3 *.ogg *.flac)"), qsTr("All files (*)")]
        onAccepted: {
            for (let i = 0; i < selectedFiles.length; ++i) {
                trackLibrary.addFileFromUrl(selectedFiles[i]);
            }
        }
    }

    ConfirmDialog {
        id: removeDialog
        title: qsTr("Remove track")
        message: qsTr("Remove “%1” from the library? It will be unbound from every channel in every preset.").arg(removeDialog.pendingName)
        onAccepted: {
            presetManager.unbindTrackEverywhere(removeDialog.pendingPath);
            trackLibrary.removeTrack(removeDialog.pendingPath);
        }

        property string pendingPath: ""
        property string pendingName: ""
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingM
        spacing: Theme.spacingS

        RowLayout {
            Layout.fillWidth: true

            Text {
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                text: qsTr("Track list")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMid
                font.bold: true
            }

            Item {
                Layout.fillWidth: true
            }

            CButton {
                symbol: qsTr("Add files")
                onClicked: importDialog.open()
            }
        }

        Rectangle {
            color: Theme.borderMid
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.topMargin: Theme.spacingS
            Layout.bottomMargin: Theme.spacingM
        }

        ListView {
            id: trackView
            clip: true
            model: trackLibrary
            spacing: Theme.spacingXS
            boundsBehavior: Flickable.StopAtBounds
            Layout.fillWidth: true
            Layout.fillHeight: true

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            Text {
                anchors.centerIn: parent
                visible: trackView.count === 0
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("The library is empty.\nPress “Add files” and then BIND\nto send a track to a channel.")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
            }

            delegate: RowLayout {
                required property string fileName
                required property int durationMs
                required property string format
                required property string filePath
                width: ListView.view ? ListView.view.width : 0
                spacing: Theme.spacingM

                Text {
                    id: dragHandle
                    text: "≡"
                    color: dragMouse.pressed ? Theme.accent : Theme.borderBright

                    MouseArea {
                        id: dragMouse
                        anchors.fill: parent
                        anchors.margins: -Theme.spacingXS
                        cursorShape: Qt.OpenHandCursor
                        preventStealing: true

                        function moveProxy(mouse) {
                            const p = dragHandle.mapToItem(dragProxy.parent, mouse.x, mouse.y);
                            dragProxy.x = p.x - dragProxy.width / 2;
                            dragProxy.y = p.y - dragProxy.height / 2;
                        }

                        onPressed: mouse => {
                            dragProxy.filePath = filePath;
                            moveProxy(mouse);
                            dragProxy.visible = true;
                            dragProxy.Drag.active = true;
                        }
                        onPositionChanged: mouse => {
                            if (dragProxy.Drag.active)
                                moveProxy(mouse);
                        }
                        onReleased: {
                            if (dragProxy.Drag.active) {
                                dragProxy.Drag.drop();
                                dragProxy.Drag.active = false;
                                dragProxy.visible = false;
                            }
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: fileName
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBase
                    elide: Text.ElideRight
                }
                Text {
                    readonly property string usage: {
                        presetManager.bindingsRevision;
                        return presetManager.bindingsFor(filePath);
                    }
                    text: usage.length > 0 ? usage : "—"
                    color: usage.length > 0 ? Theme.accent : Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    elide: Text.ElideRight
                }
                Text {
                    text: {
                        if (durationMs <= 0)
                            return "--:--";
                        const totalSec = Math.floor(durationMs / 1000);
                        const h = Math.floor(totalSec / 3600);
                        const m = Math.floor((totalSec % 3600) / 60);
                        const s = totalSec % 60;
                        const pad = n => n < 10 ? "0" + n : "" + n;
                        return h > 0 ? h + ":" + pad(m) + ":" + pad(s) : pad(m) + ":" + pad(s);
                    }
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBase
                }
                Rectangle {
                    implicitWidth: fmtLabel.implicitWidth + Theme.spacingM
                    implicitHeight: fmtLabel.implicitHeight + Theme.spacingXS
                    color: Theme.bgHover
                    border.color: Theme.borderBright
                    Text {
                        id: fmtLabel
                        anchors.centerIn: parent
                        text: format
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }
                CButton {
                    iconName: "menu"
                    onClicked: rowMenu.popup()
                    Layout.rightMargin: Theme.spacingM

                    CMenu {
                        id: rowMenu

                        CMenuItem {
                            text: qsTr("Bind")
                            onTriggered: Qt.callLater(bindMenu.popup)
                        }
                        CMenuItem {
                            text: qsTr("Remove")
                            onTriggered: {
                                removeDialog.pendingPath = filePath;
                                removeDialog.pendingName = fileName;
                                removeDialog.open();
                            }
                        }
                    }

                    CMenu {
                        id: bindMenu
                        Repeater {
                            model: audioEngine.channels
                            delegate: CMenuItem {
                                required property var modelData
                                text: modelData.name
                                onTriggered: audioEngine.bindTrack(modelData.id, filePath)
                            }
                        }
                    }
                }
            }
        }
    }
}
