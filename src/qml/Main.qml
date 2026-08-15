import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window

ApplicationWindow {
    id: root

    function newProjectDialog() {
        newProjectPopup.projectName = "";
        newProjectPopup.location = "";
        newProjectPopup.open();
    }

    function openProjectDialog() {
        openProjectFileDialog.open();
    }
    width: 1700
    height: 900
    minimumWidth: 1700
    minimumHeight: 900
    visible: true
    title: qsTr("Luma")
    color: Theme.bgBase

    flags: Qt.Window | Qt.FramelessWindowHint

    property string lastError: ""

    Component.onCompleted: {
        root.width = appSettings.windowWidth;
        root.height = appSettings.windowHeight;
        if (appSettings.windowMaximized)
            root.showMaximized();
    }

    onClosing: {
        appSettings.windowMaximized = root.visibility === Window.Maximized;
        if (root.visibility !== Window.Maximized) {
            appSettings.windowWidth = root.width;
            appSettings.windowHeight = root.height;
        }
    }

    Connections {
        target: audioEngine
        function onErrorOccurred(message) {
            root.lastError = message;
            errorTimer.restart();
        }
    }
    Timer {
        id: errorTimer
        interval: 5000
        onTriggered: root.lastError = ""
    }

    Loader {
        anchors.fill: parent
        sourceComponent: projectManager.loading ? splashComponent : (projectManager.hasProject ? mixerComponent : welcomeComponent)
    }

    Component {
        id: mixerComponent
        MixerView {}
    }

    Component {
        id: welcomeComponent
        WelcomeScreen {}
    }

    Component {
        id: splashComponent
        SplashScreen {}
    }

    Shortcut {
        sequence: (appSettings.shortcutsRevision, appSettings.shortcut("newProject"))
        enabled: !appSettings.capturing
        onActivated: root.newProjectDialog()
    }
    Shortcut {
        sequence: (appSettings.shortcutsRevision, appSettings.shortcut("openProject"))
        enabled: !appSettings.capturing
        onActivated: root.openProjectDialog()
    }

    FileDialog {
        id: openProjectFileDialog
        title: qsTr("Open project")
        nameFilters: [qsTr("Luma project (*.luma)")]
        onAccepted: projectManager.openProjectFromUrl(selectedFile)
    }

    FolderDialog {
        id: newProjectFolderDialog
        title: qsTr("Choose location")
        onAccepted: newProjectPopup.location = selectedFolder
    }

    Popup {
        id: newProjectPopup
        modal: true
        focus: true
        parent: Overlay.overlay
        anchors.centerIn: Overlay.overlay
        width: 500
        padding: Theme.spacingL
        closePolicy: Popup.CloseOnEscape

        property string projectName: ""
        property url location: ""

        background: Rectangle {
            color: Theme.bgPanel
            border.color: Theme.borderBright
            border.width: 2
            radius: Theme.radiusXS
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingL

            Text {
                text: qsTr("New project")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMid
                font.bold: true
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingM

                Text {
                    Layout.preferredWidth: 80
                    text: qsTr("Name")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBase
                }

                Item {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28

                    CTextField {
                        id: newNameField
                        onTextChanged: newProjectPopup.projectName = text
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingM

                Text {
                    Layout.preferredWidth: 80
                    text: qsTr("Location")
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBase
                }

                Text {
                    Layout.fillWidth: true
                    Layout.preferredWidth: 0
                    text: newProjectPopup.location == "" ? qsTr("(choose a folder)") : newProjectPopup.location.toString().replace("file://", "")
                    color: newProjectPopup.location == "" ? Theme.textMuted : Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSizeBase
                    elide: Text.ElideMiddle
                }

                CButton {
                    symbol: qsTr("Choose...")
                    onClicked: newProjectFolderDialog.open()
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.spacingS

                Item {
                    Layout.fillWidth: true
                }

                CButton {
                    symbol: qsTr("Create")
                    enabled: newProjectPopup.projectName.trim().length > 0 && newProjectPopup.location != ""
                    onClicked: {
                        if (projectManager.createProjectFromUrl(newProjectPopup.location, newProjectPopup.projectName))
                            newProjectPopup.close();
                    }
                }

                CButton {
                    symbol: qsTr("Cancel")
                    onClicked: newProjectPopup.close()
                }
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 32
        color: Theme.bgDangerHover
        visible: root.lastError.length > 0
        Text {
            anchors.centerIn: parent
            text: root.lastError
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: Theme.borderBright
        border.width: 2
        visible: root.visibility !== Window.Maximized
    }

    Popup {
        id: loadingOverlay
        parent: Overlay.overlay
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.NoAutoClose
        visible: presetManager.loading

        background: Rectangle {
            color: Theme.bgPanel
            border.color: Theme.borderBright
            border.width: 2
            radius: Theme.radiusXS
        }

        contentItem: ColumnLayout {
            spacing: Theme.spacingM
            BusyIndicator {
                Layout.alignment: Qt.AlignHCenter
                running: loadingOverlay.visible
            }
            Text {
                Layout.alignment: Qt.AlignHCenter
                text: qsTr("Loading preset…")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeBase
            }
        }
    }

    component ResizeHandle: MouseArea {
        property int handleEdges: 0
        acceptedButtons: Qt.LeftButton
        cursorShape: {
            if (handleEdges === Qt.LeftEdge || handleEdges === Qt.RightEdge)
                return Qt.SizeHorCursor;
            if (handleEdges === Qt.TopEdge || handleEdges === Qt.BottomEdge)
                return Qt.SizeVerCursor;
            if (handleEdges === (Qt.TopEdge | Qt.LeftEdge) || handleEdges === (Qt.BottomEdge | Qt.RightEdge))
                return Qt.SizeFDiagCursor;
            return Qt.SizeBDiagCursor;
        }
        onPressed: root.startSystemResize(handleEdges)
    }

    readonly property int resizeBorder: 6
    readonly property int resizeCorner: 12

    ResizeHandle {
        handleEdges: Qt.LeftEdge
        width: root.resizeBorder
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }
    ResizeHandle {
        handleEdges: Qt.RightEdge
        width: root.resizeBorder
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }
    ResizeHandle {
        handleEdges: Qt.TopEdge
        height: root.resizeBorder
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
    }
    ResizeHandle {
        handleEdges: Qt.BottomEdge
        height: root.resizeBorder
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
    }
    ResizeHandle {
        handleEdges: Qt.TopEdge | Qt.LeftEdge
        width: root.resizeCorner
        height: root.resizeCorner
        anchors.top: parent.top
        anchors.left: parent.left
    }
    ResizeHandle {
        handleEdges: Qt.TopEdge | Qt.RightEdge
        width: root.resizeCorner
        height: root.resizeCorner
        anchors.top: parent.top
        anchors.right: parent.right
    }
    ResizeHandle {
        handleEdges: Qt.BottomEdge | Qt.LeftEdge
        width: root.resizeCorner
        height: root.resizeCorner
        anchors.bottom: parent.bottom
        anchors.left: parent.left
    }
    ResizeHandle {
        handleEdges: Qt.BottomEdge | Qt.RightEdge
        width: root.resizeCorner
        height: root.resizeCorner
        anchors.bottom: parent.bottom
        anchors.right: parent.right
    }
}
