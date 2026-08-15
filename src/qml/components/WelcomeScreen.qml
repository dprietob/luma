import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window

Rectangle {
    id: welcome
    color: Theme.bgBase

    RowLayout {
        id: topBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: Theme.spacingL
        height: 48
        spacing: 0

        MouseArea {
            Layout.fillWidth: true
            Layout.fillHeight: true
            onPressed: if (Window.window)
                Window.window.startSystemMove()
        }

        WindowButtons {}
    }

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * Theme.spacingXL, 720)
        spacing: Theme.spacingL

        Text {
            text: "LUMA"
            color: Theme.accent
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeLarge * 2
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: qsTr("Create a new project or open an existing one.")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMid
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: Theme.spacingL
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: Theme.spacingL

            CButton {
                symbol: qsTr("New project")
                big: true
                Layout.preferredWidth: 220
                Layout.preferredHeight: 48
                onClicked: if (Window.window)
                    Window.window.newProjectDialog()
            }

            CButton {
                symbol: qsTr("Open project")
                big: true
                Layout.preferredWidth: 220
                Layout.preferredHeight: 48
                onClicked: if (Window.window)
                    Window.window.openProjectDialog()
            }
        }

        Text {
            text: qsTr("Recent projects")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
            font.bold: true
            Layout.topMargin: Theme.spacingL
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 220
            color: Theme.bgPanel
            border.color: Theme.borderSubtle
            border.width: 1
            radius: Theme.radiusXS

            ListView {
                id: recentView
                anchors.fill: parent
                anchors.margins: Theme.spacingS
                clip: true
                spacing: Theme.spacingXS
                model: projectManager.recentProjects
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }

                Text {
                    anchors.centerIn: parent
                    visible: recentView.count === 0
                    horizontalAlignment: Text.AlignHCenter
                    text: qsTr("No recent projects yet.")
                    color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBase
                }

                delegate: Rectangle {
                    id: recentItem
                    required property var modelData
                    width: ListView.view.width
                    height: 44
                    radius: Theme.radiusXS
                    color: recentMouse.containsMouse ? Theme.bgHover : Theme.bgSubPanel
                    border.color: Theme.borderSubtle
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingS
                        spacing: Theme.spacingS

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            Text {
                                Layout.fillWidth: true
                                text: recentItem.modelData.name
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeBase
                                elide: Text.ElideRight
                            }
                            Text {
                                Layout.fillWidth: true
                                text: recentItem.modelData.path
                                color: Theme.textMuted
                                font.family: Theme.fontFamily
                                font.pixelSize: Theme.fontSizeSmall
                                elide: Text.ElideMiddle
                            }
                        }

                        CButton {
                            iconName: "close"
                            danger: true
                            Layout.preferredWidth: 34
                            onClicked: {
                                removeRecentDialog.pendingPath = recentItem.modelData.path;
                                removeRecentDialog.pendingName = recentItem.modelData.name;
                                removeRecentDialog.open();
                            }
                        }
                    }

                    MouseArea {
                        id: recentMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton
                        z: -1
                        onClicked: projectManager.openProject(recentItem.modelData.path)
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.topMargin: Theme.spacingM

            CButton {
                symbol: qsTr("About")
                big: true
                Layout.preferredWidth: 220
                Layout.preferredHeight: 48
                onClicked: aboutDialog.open()
            }
        }
    }

    ConfirmDialog {
        id: removeRecentDialog
        title: qsTr("Remove from recent")
        message: qsTr("Remove “%1” from the recent projects list? The project files will not be deleted.").arg(removeRecentDialog.pendingName)
        onAccepted: projectManager.removeRecent(removeRecentDialog.pendingPath)

        property string pendingPath: ""
        property string pendingName: ""
    }

    Popup {
        id: aboutDialog
        modal: true
        focus: true
        parent: Overlay.overlay
        anchors.centerIn: Overlay.overlay
        width: 460
        padding: Theme.spacingXL
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: Theme.bgPanel
            border.color: Theme.borderBright
            border.width: 2
            radius: Theme.radiusXS
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingM

            Image {
                source: "qrc:/icons/luma_128.png"
                sourceSize.width: 96
                sourceSize.height: 96
                Layout.preferredWidth: 96
                Layout.preferredHeight: 96
                Layout.alignment: Qt.AlignHCenter
                fillMode: Image.PreserveAspectFit
                smooth: true
            }

            Text {
                text: "Luma"
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeLarge
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: qsTr("Version %1").arg(Qt.application.version)
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spacingS
                text: qsTr("Real-time multitrack audio mixer for Linux.")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMid
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: qsTr("License: %1").arg("GPL-3.0-or-later")
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: "© 2026 Daniel Prieto"
                color: Theme.textMuted
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                text: '<a href="https://github.com/dprietob/luma" style="color: #e8601c;">github.com/dprietob/luma</a>'
                textFormat: Text.RichText
                linkColor: Theme.accent
                color: Theme.accent
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
                Layout.alignment: Qt.AlignHCenter
                onLinkActivated: link => Qt.openUrlExternally(link)

                HoverHandler {
                    cursorShape: Qt.PointingHandCursor
                }
            }

            CButton {
                symbol: qsTr("Close")
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: Theme.spacingS
                Layout.preferredWidth: 120
                onClicked: aboutDialog.close()
            }
        }
    }
}
