import QtQuick
import QtQuick.Layouts

Rectangle {
    id: splash
    color: Theme.bgBase

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 2 * Theme.spacingXL, 420)
        spacing: Theme.spacingL

        Image {
            source: "qrc:/icons/luma_128.png"
            sourceSize.width: 112
            sourceSize.height: 112
            Layout.preferredWidth: 112
            Layout.preferredHeight: 112
            Layout.alignment: Qt.AlignHCenter
            fillMode: Image.PreserveAspectFit
            smooth: true
        }

        Text {
            text: "LUMA"
            color: Theme.accent
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeLarge
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
        }

        Text {
            text: qsTr("Starting up… loading your project.")
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: Theme.spacingS
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 6
            radius: 3
            color: Theme.bgControl
            border.color: Theme.borderSubtle
            border.width: 1

            Rectangle {
                height: parent.height
                width: parent.width * Math.max(0, Math.min(1, sessionManager.loadProgress))
                radius: parent.radius
                color: Theme.accent
            }
        }
    }
}
