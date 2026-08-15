pragma Singleton
import QtQuick

QtObject {
    readonly property var modes: [
        {
            label: qsTr("Linear"),
            easingType: Easing.Linear
        },
        {
            label: qsTr("Ease"),
            easingType: Easing.InOutQuad
        },
        {
            label: qsTr("Ease In"),
            easingType: Easing.InQuad
        },
        {
            label: qsTr("Ease Out"),
            easingType: Easing.OutQuad
        },
        {
            label: qsTr("Ease In-Out"),
            easingType: Easing.InOutCubic
        },
        {
            label: qsTr("Cubic Bezier"),
            easingType: Easing.Bezier,
            bezier: [0.68, -0.55, 0.27, 1.55, 1.0, 1.0]
        }
    ]

    function count() {
        return modes.length;
    }
    function labelAt(index) {
        return (modes[index] || modes[0]).label;
    }

    function sample(index, t) {
        switch (index) {
        case 1:
            return t < 0.5 ? 2 * t * t : 1 - Math.pow(-2 * t + 2, 2) / 2;
        case 2:
            return t * t;
        case 3:
            return 1 - (1 - t) * (1 - t);
        case 4:
            return t < 0.5 ? 4 * t * t * t : 1 - Math.pow(-2 * t + 2, 3) / 2;
        case 5:
            return cubicBezierY(0.68, -0.55, 0.27, 1.55, t);
        default:
            return t;
        }
    }

    function cubicBezierY(x1, y1, x2, y2, x) {
        function bx(u) {
            const v = 1 - u;
            return 3 * v * v * u * x1 + 3 * v * u * u * x2 + u * u * u;
        }
        function by(u) {
            const v = 1 - u;
            return 3 * v * v * u * y1 + 3 * v * u * u * y2 + u * u * u;
        }
        let lo = 0.0, hi = 1.0, u = x;
        for (let i = 0; i < 24; ++i) {
            u = (lo + hi) / 2;
            const xu = bx(u);
            if (Math.abs(xu - x) < 0.0005)
                break;
            if (xu < x)
                lo = u;
            else
                hi = u;
        }
        return by(u);
    }
}
