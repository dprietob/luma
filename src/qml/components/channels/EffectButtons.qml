import QtQuick
import QtQuick.Layouts

RowLayout {
    id: effects
    spacing: Theme.spacingS
    Layout.alignment: Qt.AlignHCenter

    property bool fxActive: false
    property bool eqActive: false

    signal fxRequested
    signal eqRequested

    CButton {
        id: fxButton
        iconName: "effects"
        square: true
        bgColor: effects.fxActive ? Theme.accent : "transparent"
        onClicked: effects.fxRequested()
    }

    CButton {
        id: eqButton
        iconName: "equalizer"
        square: true
        bgColor: effects.eqActive ? Theme.accent : "transparent"
        onClicked: effects.eqRequested()
    }
}
