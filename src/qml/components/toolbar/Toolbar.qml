import QtQuick
import QtQuick.Layouts
import QtQuick.Window

Rectangle {
    id: toolbar
    color: Theme.bgBase

    property bool reorderMode: false

    MouseArea {
        anchors.fill: parent
        onPressed: if (Window.window)
            Window.window.startSystemMove()
    }

    Shortcut {
        sequence: (appSettings.shortcutsRevision, appSettings.shortcut("masterMin"))
        enabled: !appSettings.capturing
        onActivated: masterBus.channelMasterVolume = 0
    }
    Shortcut {
        sequence: (appSettings.shortcutsRevision, appSettings.shortcut("panic"))
        enabled: !appSettings.capturing
        onActivated: audioEngine.panic()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spacingL
        anchors.rightMargin: Theme.spacingL
        spacing: Theme.spacingL

        Text {
            text: "LUMA"
            color: Theme.accent
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeLarge
            font.bold: true
        }

        CButton {
            symbol: qsTr("PROJECTS")
            fontSmall: true
            Accessible.name: qsTr("Close project and go to the welcome screen")
            onClicked: projectManager.closeProject()
        }

        Text {
            text: projectManager.projectName
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
            elide: Text.ElideRight
            Layout.maximumWidth: 160
        }

        Text {
            text: qsTr("MASTER")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeSmall
        }

        HFader {
            size: 450
            value: masterBus.channelMasterVolume
            vuLeft: masterBus.masterVuLeft
            vuRight: masterBus.masterVuRight
            onMoved: value => masterBus.channelMasterVolume = value
        }

        PanicButton {}

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        OptionsArea {}
        PresetsArea {}
        WindowButtons {}
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: Theme.borderMid
    }
}
