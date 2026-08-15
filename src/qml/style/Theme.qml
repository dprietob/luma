pragma Singleton
import QtQuick

QtObject {
    readonly property color bgBase: "#080808"
    readonly property color bgPanel: "#151515"
    readonly property color bgSubPanel: "#1f1f1f"
    readonly property color bgControl: "#252525"
    readonly property color bgHover: "#2f2f2f"
    readonly property color bgDanger: "#401A13"
    readonly property color bgDangerHover: "#61291f"

    readonly property color borderSubtle: "#000000"
    readonly property color borderMid: "#3d3d3d"
    readonly property color borderBright: "#555555"

    readonly property color textPrimary: "#eeeeee"
    readonly property color textSecondary: "#aaaaaa"
    readonly property color textMuted: "#666666"

    readonly property color accent: "#e8601c"

    readonly property color vuGreen: "#44cc44"
    readonly property color vuYellow: "#cccc00"
    readonly property color vuRed: "#cc2200"

    readonly property color trackLoaded: "#44cc44"
    readonly property color trackEmpty: "#444444"

    readonly property color controlTrack: "#000000"
    readonly property color vuBackground: "#1a1a1a"

    readonly property string fontFamily: "JetBrains Mono"
    readonly property int fontSizeSmall: 9
    readonly property int fontSizeBase: 11
    readonly property int fontSizeMid: 13
    readonly property int fontSizeLarge: 16

    readonly property int spacingXS: 2
    readonly property int spacingS: 4
    readonly property int spacingM: 8
    readonly property int spacingL: 16
    readonly property int spacingXL: 24

    readonly property int radiusXS: 2
}
