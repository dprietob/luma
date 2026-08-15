import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Dialog {
    id: confirmDialog
    modal: true
    focus: true
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay

    property string message: ""

    onOpened: yesButton.forceActiveFocus()

    header: Item {
        implicitHeight: titleLabel.implicitHeight + 2 * Theme.spacingL

        Rectangle {
            anchors.fill: parent
            anchors.topMargin: dialogBg.border.width
            anchors.leftMargin: dialogBg.border.width
            anchors.rightMargin: dialogBg.border.width
            color: Theme.bgBase
            topLeftRadius: dialogBg.radius
            topRightRadius: dialogBg.radius

            Text {
                id: titleLabel
                anchors.fill: parent
                anchors.margins: Theme.spacingL
                text: confirmDialog.title
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSizeMid
                font.bold: true
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    contentItem: Text {
        text: confirmDialog.message
        color: Theme.textPrimary
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSizeMid
        wrapMode: Text.WordWrap
    }

    footer: Item {
        implicitHeight: footerRow.implicitHeight + 2 * Theme.spacingL

        RowLayout {
            id: footerRow
            anchors.fill: parent
            anchors.margins: Theme.spacingL
            spacing: Theme.spacingM

            Item {
                Layout.fillWidth: true
            }

            ConfirmButton {
                text: qsTr("No")
                onClicked: confirmDialog.reject()
            }

            ConfirmButton {
                id: yesButton
                text: qsTr("Yes")
                focus: true
                onClicked: confirmDialog.accept()
                Keys.onReturnPressed: confirmDialog.accept()
                Keys.onEnterPressed: confirmDialog.accept()
            }
        }
    }

    background: Rectangle {
        id: dialogBg
        color: Theme.bgPanel
        border.color: Theme.borderBright
        border.width: 2
        radius: Theme.radiusXS
    }

    component ConfirmButton: CButton {
        id: confirmButton

        contentItem: Text {
            text: confirmButton.text
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeMid
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            implicitWidth: 48
            implicitHeight: 25
            color: Theme.bgControl
            border.color: (confirmButton.activeFocus || confirmButton.hovered) ? Theme.accent : Theme.borderSubtle
            border.width: 1
            radius: Theme.radiusXS
        }
    }
}
