import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: mixerView
    spacing: 0

    Toolbar {
        id: mainToolbar
        Layout.fillWidth: true
        Layout.preferredHeight: 48
    }

    SplitView {
        id: mainSplit
        Layout.fillWidth: true
        Layout.fillHeight: true
        orientation: Qt.Horizontal

        handle: Rectangle {
            implicitWidth: Theme.spacingM
            color: Theme.bgBase
        }

        SplitView {
            SplitView.preferredWidth: 400
            SplitView.minimumWidth: 400
            orientation: Qt.Vertical

            handle: Rectangle {
                implicitHeight: Theme.spacingM
                color: Theme.bgBase
            }

            TrackList {
                SplitView.fillHeight: true
                SplitView.minimumHeight: 300
            }

            FlowList {
                SplitView.preferredHeight: 460
                SplitView.minimumHeight: 300
            }
        }

        ChannelsArea {
            SplitView.fillWidth: true
            SplitView.minimumWidth: 1000
            reorderMode: mainToolbar.reorderMode
            gridMode: audioEngine.gridMode
        }
    }
}
