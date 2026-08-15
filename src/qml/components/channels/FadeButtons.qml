import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: fade

    Layout.alignment: Qt.AlignHCenter

    property bool isFadingIn: false
    property bool isFadingOut: false
    property int fadeSeconds: 2
    property int fadeMode: 0

    signal fadeInRequested
    signal fadeOutRequested
    signal fadeSecondsEdited(int seconds)
    signal fadeModeSelected(int mode)

    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        spacing: Theme.spacingS

        TextField {
            id: fadeSpeedField
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            Layout.leftMargin: Theme.spacingS
            horizontalAlignment: TextInput.AlignHCenter
            verticalAlignment: TextInput.AlignVCenter
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSizeBase
            text: fade.fadeSeconds
            ToolTip.text: qsTr("Fade duration (s)")
            inputMethodHints: Qt.ImhDigitsOnly
            validator: IntValidator {
                bottom: 0
                top: 60
            }

            background: Rectangle {
                color: Theme.bgControl
                border.color: fadeSpeedField.activeFocus ? Theme.accent : Theme.borderSubtle
                border.width: 1
                radius: Theme.radiusXS
            }

            onTextEdited: {
                const v = parseInt(text, 10);
                if (!isNaN(v))
                    fade.fadeSecondsEdited(v);
            }
        }

        CButton {
            id: fadeModeButton
            onClicked: fadeModePopup.open()
            Layout.rightMargin: Theme.spacingS

            contentItem: CurveCanvas {
                implicitWidth: 12
                implicitHeight: 12
                modeIndex: fade.fadeMode
                curveColor: Theme.accent
            }
        }
    }

    Popup {
        id: fadeModePopup
        parent: fadeModeButton
        y: fadeModeButton.height + Theme.spacingXS
        padding: Theme.spacingS
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: Theme.bgPanel
            border.color: Theme.borderBright
            border.width: 1
            radius: Theme.radiusXS
        }

        contentItem: GridLayout {
            columns: 2
            columnSpacing: Theme.spacingS
            rowSpacing: Theme.spacingS

            Repeater {
                model: FadeModes.count()
                delegate: Rectangle {
                    id: option
                    implicitWidth: 78
                    implicitHeight: 56
                    color: selected ? Theme.bgHover : Theme.bgControl
                    border.color: selected ? Theme.accent : Theme.borderSubtle
                    border.width: 1
                    radius: Theme.radiusXS

                    required property int index
                    readonly property bool selected: index === fade.fadeMode

                    Column {
                        anchors.centerIn: parent
                        spacing: Theme.spacingXS

                        CurveCanvas {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 62
                            height: 30
                            modeIndex: option.index
                            curveColor: option.selected ? Theme.accent : Theme.textSecondary
                        }

                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: FadeModes.labelAt(option.index)
                            color: option.selected ? Theme.textPrimary : Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: Theme.fontSizeSmall
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            fade.fadeModeSelected(option.index);
                            fadeModePopup.close();
                        }
                    }
                }
            }
        }
    }

    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        spacing: Theme.spacingS

        CButton {
            id: fadeInButton
            iconName: "fadein"
            square: true
            bgColor: fade.isFadingIn ? Theme.accent : "transparent"
            onClicked: fade.fadeInRequested()
        }

        CButton {
            id: fadeOutButton
            iconName: "fadeout"
            square: true
            bgColor: fade.isFadingOut ? Theme.accent : "transparent"
            onClicked: fade.fadeOutRequested()
        }
    }

    component CurveCanvas: Canvas {
        property int modeIndex: 0
        property color curveColor: Theme.accent
        onModeIndexChanged: requestPaint()
        onCurveColorChanged: requestPaint()
        onPaint: {
            const ctx = getContext("2d");
            ctx.reset();
            ctx.lineWidth = 1.5;
            ctx.strokeStyle = curveColor;
            ctx.lineJoin = "round";
            ctx.beginPath();
            const pad = 1;
            const w = width - 2 * pad;
            const h = height - 2 * pad;
            const n = 32;
            for (let i = 0; i <= n; ++i) {
                const t = i / n;
                const y = Math.max(0, Math.min(1, FadeModes.sample(modeIndex, t)));
                const px = pad + t * w;
                const py = pad + (1 - y) * h;
                if (i === 0)
                    ctx.moveTo(px, py);
                else
                    ctx.lineTo(px, py);
            }
            ctx.stroke();
        }
    }
}
