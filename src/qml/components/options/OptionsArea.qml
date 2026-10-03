import QtQuick
import QtQuick.Layouts

Rectangle {
    id: optionsArea
    color: Theme.bgBase

    implicitWidth: buttonsRow.implicitWidth
    implicitHeight: buttonsRow.implicitHeight

    RowLayout {
        id: buttonsRow
        anchors.fill: parent
        spacing: Theme.spacingS

        CButton {
            iconName: "devices"
            fontSmall: true
            onClicked: audioOutputDialog.open()
        }

        CButton {
            iconName: "number"
            fontSmall: true
            onClicked: channelCountDialog.open()
        }

        CButton {
            iconName: "reorder"
            fontSmall: true
            checkable: true
            checked: toolbar.reorderMode
            onToggled: toolbar.reorderMode = checked
        }

        CButton {
            iconName: "grid"
            fontSmall: true
            checkable: true
            checked: audioEngine.gridMode
            onToggled: audioEngine.gridMode = checked
        }

        CButton {
            iconName: "settings"
            fontSmall: true
            onClicked: preferencesDialog.open()
        }

        Item {
            Layout.preferredWidth: Theme.spacingXL
        }
    }

    Shortcut {
        sequence: (appSettings.shortcutsRevision, appSettings.shortcut("audioOutput"))
        enabled: !appSettings.capturing
        onActivated: audioOutputDialog.open()
    }
    Shortcut {
        sequence: (appSettings.shortcutsRevision, appSettings.shortcut("channelCount"))
        enabled: !appSettings.capturing
        onActivated: channelCountDialog.open()
    }
    Shortcut {
        sequence: (appSettings.shortcutsRevision, appSettings.shortcut("reorder"))
        enabled: !appSettings.capturing
        onActivated: toolbar.reorderMode = !toolbar.reorderMode
    }
    Shortcut {
        sequence: (appSettings.shortcutsRevision, appSettings.shortcut("grid"))
        enabled: !appSettings.capturing
        onActivated: audioEngine.gridMode = !audioEngine.gridMode
    }
    Shortcut {
        sequence: (appSettings.shortcutsRevision, appSettings.shortcut("preferences"))
        enabled: !appSettings.capturing
        onActivated: preferencesDialog.open()
    }

    AudioOutputDialog {
        id: audioOutputDialog
    }

    ChannelCountDialog {
        id: channelCountDialog
    }

    PreferencesDialog {
        id: preferencesDialog
    }
}
