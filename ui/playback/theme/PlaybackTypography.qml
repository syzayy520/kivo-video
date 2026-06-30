pragma Singleton
import QtQuick 6.5

QtObject {
    readonly property string uiFamily: "Segoe UI Variable"
    readonly property string fallbackFamily: "Segoe UI"
    readonly property string cjkFamily: "Microsoft YaHei UI"

    readonly property int titleSize: 21
    readonly property int bodySize: 14
    readonly property int captionSize: 12
    readonly property int timeSize: 13
    readonly property int subtitlePrimarySize: 31
    readonly property int subtitleSecondarySize: 19
}
