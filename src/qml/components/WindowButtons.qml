import QtQuick.Layouts
import QtQuick.Window

RowLayout {
    id: windowButtons
    spacing: 0

    readonly property Window win: Window.window
    readonly property bool maximized: win ? win.visibility === Window.Maximized : false

    CButton {
        iconName: "minimize"
        onClicked: if (windowButtons.win)
            windowButtons.win.showMinimized()
    }

    CButton {
        iconName: windowButtons.maximized ? "restore" : "maximize"
        iconSize: 12
        onClicked: {
            if (!windowButtons.win)
                return;
            if (windowButtons.maximized)
                windowButtons.win.showNormal();
            else
                windowButtons.win.showMaximized();
        }
    }

    CButton {
        iconName: "close"
        danger: true
        onClicked: if (windowButtons.win)
            windowButtons.win.close()
    }
}
