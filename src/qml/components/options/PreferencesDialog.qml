import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Popup {
    id: preferencesDialog
    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    width: 520
    padding: Theme.spacingL
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string capturingId: ""

    onClosed: {
        preferencesDialog.capturingId = "";
        appSettings.capturing = false;
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
            text: qsTr("Preferences")
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMid
            font.bold: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingM

            Text {
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                text: qsTr("Open the last project on startup")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
                wrapMode: Text.WordWrap
            }

            CButton {
                symbol: appSettings.openLastOnStartup ? "ON" : "OFF"
                checkable: true
                checked: appSettings.openLastOnStartup
                Layout.preferredWidth: 60
                onToggled: appSettings.openLastOnStartup = checked
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingM

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                spacing: 0

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Language")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBase
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Applied after restart.")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.WordWrap
                }
            }

            CButton {
                id: languageButton
                Layout.preferredWidth: 140
                symbol: {
                    const list = appSettings.availableLanguages();
                    for (var i = 0; i < list.length; ++i)
                        if (list[i].code === appSettings.language)
                            return list[i].name;
                    return appSettings.language;
                }
                onClicked: languageMenu.open()

                CMenu {
                    id: languageMenu
                    Repeater {
                        model: appSettings.availableLanguages()
                        delegate: CMenuItem {
                            required property var modelData
                            text: modelData.name
                            onTriggered: appSettings.language = modelData.code
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingM

            Text {
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                text: qsTr("Audio cache size")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
            }

            Item {
                Layout.preferredWidth: 80
                Layout.preferredHeight: 28

                CTextField {
                    id: cacheField
                    text: "" + appSettings.audioCacheMB
                    validator: IntValidator {
                        bottom: 0
                        top: 65536
                    }
                    inputMethodHints: Qt.ImhDigitsOnly
                    onEditingFinished: appSettings.audioCacheMB = parseInt(text) || 0
                }
            }

            Text {
                text: "MB"
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingM

            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredWidth: 0
                spacing: 0

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Rewarm current project cache")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBase
                }
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Decodes all project tracks into memory now.")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.WordWrap
                }
            }

            BusyIndicator {
                Layout.preferredWidth: 26
                Layout.preferredHeight: 26
                running: audioEngine.warming
                visible: audioEngine.warming
            }

            CButton {
                symbol: audioEngine.warming ? qsTr("Warming...") : qsTr("Rewarm")
                Layout.preferredWidth: 100
                enabled: trackLibrary.count > 0 && !audioEngine.warming
                onClicked: audioEngine.warmTrackCache()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.borderMid
        }

        Text {
            text: qsTr("Keyboard shortcuts")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
            font.bold: true
        }

        ListView {
            id: shortcutsView
            Layout.fillWidth: true
            Layout.preferredHeight: 240
            clip: true
            spacing: Theme.spacingXS
            model: appSettings.shortcutList()
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
            }

            delegate: RowLayout {
                required property var modelData
                width: ListView.view.width
                spacing: Theme.spacingM

                Text {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    text: modelData.label
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBase
                    elide: Text.ElideRight
                }

                Rectangle {
                    id: keyBtn
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 28
                    radius: Theme.radiusXS
                    readonly property bool active: preferencesDialog.capturingId === modelData.id
                    color: keyBtn.active ? Theme.accent : (capMouse.containsMouse ? Theme.bgHover : Theme.bgControl)
                    border.color: keyBtn.active ? Theme.accent : Theme.borderSubtle
                    border.width: 1
                    focus: keyBtn.active

                    Text {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingS
                        text: keyBtn.active ? qsTr("Press keys…") : (appSettings.shortcutsRevision, appSettings.shortcut(modelData.id))
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSizeBase
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }

                    Keys.onPressed: event => {
                        if (!keyBtn.active)
                            return;
                        if (event.key === Qt.Key_Escape) {
                            preferencesDialog.capturingId = "";
                            appSettings.capturing = false;
                            event.accepted = true;
                            return;
                        }
                        const seq = appSettings.sequenceFromEvent(event.key, event.modifiers);
                        if (seq.length > 0) {
                            appSettings.setShortcut(modelData.id, seq);
                            preferencesDialog.capturingId = "";
                            appSettings.capturing = false;
                        }
                        event.accepted = true;
                    }

                    MouseArea {
                        id: capMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            preferencesDialog.capturingId = modelData.id;
                            appSettings.capturing = true;
                            keyBtn.forceActiveFocus();
                        }
                    }
                }

                CButton {
                    iconName: "reset"
                    Layout.preferredWidth: 44
                    onClicked: {
                        appSettings.resetShortcut(modelData.id);
                        preferencesDialog.capturingId = "";
                        appSettings.capturing = false;
                    }
                }
            }
        }
    }
}
