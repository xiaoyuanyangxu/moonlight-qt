import QtQuick 2.9
import QtQuick.Controls 2.2

import StreamingPreferences 1.0
import ComputerManager 1.0
import SdlGamepadKeyNavigation 1.0
import SystemProperties 1.0

Flickable {
    id: settingsPage
    objectName: "SettingsCloud"

    boundsBehavior: Flickable.OvershootBounds

    contentWidth: settingsColumn1.width > settingsColumn2.width ? settingsColumn1.width : settingsColumn2.width
    contentHeight: settingsColumn1.height > settingsColumn2.height ? settingsColumn1.height : settingsColumn2.height

    ScrollBar.vertical: ScrollBar {
        anchors {
            left: parent.right
            leftMargin: -10
        }
    }

    StackView.onActivated: {
        // This enables Tab and BackTab based navigation rather than arrow keys.
        // It is required to shift focus between controls on the settings page.
        SdlGamepadKeyNavigation.setUiNavMode(true)

        // Highlight the first item if a gamepad is connected
        if (SdlGamepadKeyNavigation.getConnectedGamepads() > 0) {
            resolutionComboBox.forceActiveFocus(Qt.TabFocus)
        }
    }

    StackView.onDeactivating: {
        SdlGamepadKeyNavigation.setUiNavMode(false)

        // Save the prefs so the Session can observe the changes
        StreamingPreferences.save()
    }

    Column {
        padding: 10
        id: settingsColumn1
        width: settingsPage.width / 2
        spacing: 5


        GroupBox {
            id: basicSettingsGroupBox
            width: (parent.width - (parent.leftPadding + parent.rightPadding))
            padding: 12
            title: "<font color=\"skyblue\">Basic Settings</font>"
            font.pointSize: 12

            Column {
                anchors.fill: parent
                spacing: 5

                Label {
                    width: parent.width
                    id: resFPStitle
                    text: qsTr("Resolution and FPS")
                    font.pointSize: 12
                    wrapMode: Text.Wrap
                }

                Label {
                    width: parent.width
                    id: resFPSdesc
                    text: qsTr("Setting values too high for your PC or network connection may cause lag, stuttering, or errors.")
                    font.pointSize: 9
                    wrapMode: Text.Wrap
                }

                Row {
                    spacing: 5
                    width: parent.width

                    AutoResizingComboBox {
                        // ignore setting the index at first, and actually set it when the component is loaded
                        Component.onCompleted: {
                            // Add native resolutions for all attached displays
                            var done = false
                            for (var displayIndex = 0; !done; displayIndex++) {
                                for (var displayResIndex = 0; displayResIndex < 2; displayResIndex++) {
                                    var screenRect;

                                    // Some platforms have different desktop resolutions
                                    // and native resolutions (like macOS with Retina displays)
                                    if (displayResIndex === 0) {
                                        screenRect = SystemProperties.getDesktopResolution(displayIndex)
                                    }
                                    else {
                                        screenRect = SystemProperties.getNativeResolution(displayIndex)
                                    }

                                    if (screenRect.width === 0) {
                                        // Exceeded max count of displays
                                        done = true
                                        break
                                    }

                                    var indexToAdd = 0
                                    for (var j = 0; j < resolutionComboBox.count; j++) {
                                        var existing_width = parseInt(resolutionListModel.get(j).video_width);
                                        var existing_height = parseInt(resolutionListModel.get(j).video_height);

                                        if (screenRect.width === existing_width && screenRect.height === existing_height) {
                                            // Duplicate entry, skip
                                            indexToAdd = -1
                                            break
                                        }
                                        else if (screenRect.width * screenRect.height > existing_width * existing_height) {
                                            // Candidate entrypoint after this entry
                                            indexToAdd = j + 1
                                        }
                                    }

                                    // Insert this display's resolution if it's not a duplicate
                                    if (indexToAdd >= 0) {
                                        resolutionListModel.insert(indexToAdd,
                                                                   {
                                                                       "text": "Native ("+screenRect.width+"x"+screenRect.height+")",
                                                                       "video_width": ""+screenRect.width,
                                                                       "video_height": ""+screenRect.height
                                                                   })
                                    }
                                }
                            }

                            // Prune resolutions that are over the decoder's maximum
                            var max_pixels = SystemProperties.maximumResolution.width * SystemProperties.maximumResolution.height;
                            if (max_pixels > 0) {
                                for (var j = 0; j < resolutionComboBox.count; j++) {
                                    var existing_width = parseInt(resolutionListModel.get(j).video_width);
                                    var existing_height = parseInt(resolutionListModel.get(j).video_height);

                                    if (existing_width * existing_height > max_pixels) {
                                        resolutionListModel.remove(j)
                                        j--
                                    }
                                }
                            }

                            // load the saved width/height, and iterate through the ComboBox until a match is found
                            // and set it to that index.
                            var saved_width = StreamingPreferences.width
                            var saved_height = StreamingPreferences.height
                            currentIndex = 0
                            for (var i = 0; i < resolutionListModel.count; i++) {
                                var el_width = parseInt(resolutionListModel.get(i).video_width);
                                var el_height = parseInt(resolutionListModel.get(i).video_height);

                                // Pick the highest value lesser or equal to the saved resolution
                                if (saved_width * saved_height >= el_width * el_height) {
                                    currentIndex = i
                                }
                            }

                            // Persist the selected value
                            activated(currentIndex)
                        }

                        id: resolutionComboBox
                        maximumWidth: parent.width / 2
                        textRole: "text"
                        model: ListModel {
                            id: resolutionListModel
                            // Other elements may be added at runtime
                            // based on attached display resolution
                            ListElement {
                                text: "720p"
                                video_width: "1280"
                                video_height: "720"
                            }
                            ListElement {
                                text: "1080p"
                                video_width: "1920"
                                video_height: "1080"
                            }
                            ListElement {
                                text: "1440p"
                                video_width: "2560"
                                video_height: "1440"
                            }
                            ListElement {
                                text: "4K"
                                video_width: "3840"
                                video_height: "2160"
                            }
                        }
                        // ::onActivated must be used, as it only listens for when the index is changed by a human
                        onActivated : {
                            var selectedWidth = parseInt(resolutionListModel.get(currentIndex).video_width)
                            var selectedHeight = parseInt(resolutionListModel.get(currentIndex).video_height)

                            // Only modify the bitrate if the values actually changed
                            if (StreamingPreferences.width !== selectedWidth || StreamingPreferences.height !== selectedHeight) {
                                StreamingPreferences.width = selectedWidth
                                StreamingPreferences.height = selectedHeight

                                StreamingPreferences.bitrateKbps = StreamingPreferences.getDefaultBitrate(StreamingPreferences.width,
                                                                                                          StreamingPreferences.height,
                                                                                                          StreamingPreferences.fps);
                                slider.value = StreamingPreferences.bitrateKbps
                            }
                        }
                    }

                    AutoResizingComboBox {
                        function createModel() {
                            var fpsListModel = Qt.createQmlObject('import QtQuick 2.0; ListModel {}', parent, '')

                            var max_fps = SystemProperties.maximumStreamingFrameRate

                            // Default entries
                            fpsListModel.append({"text": "30 FPS", "video_fps": "30"})
                            fpsListModel.append({"text": "60 FPS", "video_fps": "60"})

                            // Add unsupported FPS values that come before the display max FPS
                            if (StreamingPreferences.unsupportedFps) {
                                if (max_fps > 90) {
                                    fpsListModel.append({"text": "90 FPS (Unsupported)", "video_fps": "90"})
                                }
                                if (max_fps > 120) {
                                    fpsListModel.append({"text": "120 FPS (Unsupported)", "video_fps": "120"})
                                }
                            }

                            // Use 64 as the cutoff for adding a separate option to
                            // handle wonky displays that report just over 60 Hz.
                            if (max_fps > 64) {
                                // Mark any FPS value greater than 120 as unsupported
                                if (StreamingPreferences.unsupportedFps && max_fps > 120) {
                                    fpsListModel.append({"text": max_fps+" FPS (Unsupported)", "video_fps": ""+max_fps})
                                }
                                else if (max_fps > 120) {
                                    fpsListModel.append({"text": "120 FPS", "video_fps": "120"})
                                }
                                else {
                                    fpsListModel.append({"text": max_fps+" FPS", "video_fps": ""+max_fps})
                                }
                            }

                            // Add unsupported FPS values that come after the display max FPS
                            if (StreamingPreferences.unsupportedFps) {
                                if (max_fps < 90) {
                                    fpsListModel.append({"text": "90 FPS (Unsupported)", "video_fps": "90"})
                                }
                                if (max_fps < 120) {
                                    fpsListModel.append({"text": "120 FPS (Unsupported)", "video_fps": "120"})
                                }
                            }

                            return fpsListModel
                        }

                        function reinitialize() {
                            model = createModel()

                            var saved_fps = StreamingPreferences.fps
                            currentIndex = 0
                            for (var i = 0; i < model.count; i++) {
                                var el_fps = parseInt(model.get(i).video_fps);

                                // Pick the highest value lesser or equal to the saved FPS
                                if (saved_fps >= el_fps) {
                                    currentIndex = i
                                }
                            }

                            // Persist the selected value
                            activated(currentIndex)
                        }

                        // ignore setting the index at first, and actually set it when the component is loaded
                        Component.onCompleted: {
                            reinitialize()
                        }

                        id: fpsComboBox
                        maximumWidth: parent.width / 2
                        textRole: "text"
                        // ::onActivated must be used, as it only listens for when the index is changed by a human
                        onActivated : {
                            var selectedFps = parseInt(model.get(currentIndex).video_fps)

                            // Only modify the bitrate if the values actually changed
                            if (StreamingPreferences.fps !== selectedFps) {
                                StreamingPreferences.fps = selectedFps

                                StreamingPreferences.bitrateKbps = StreamingPreferences.getDefaultBitrate(StreamingPreferences.width,
                                                                                                          StreamingPreferences.height,
                                                                                                          StreamingPreferences.fps);
                                slider.value = StreamingPreferences.bitrateKbps
                            }
                        }
                    }
                }

                Label {
                    width: parent.width
                    id: bitrateTitle
                    text: qsTr("Video bitrate: ")
                    font.pointSize: 12
                    wrapMode: Text.Wrap
                }

                Label {
                    width: parent.width
                    id: bitrateDesc
                    text: qsTr("Lower the bitrate on slower connections. Raise the bitrate to increase image quality.")
                    font.pointSize: 9
                    wrapMode: Text.Wrap
                }

                Slider {
                    id: slider

                    value: StreamingPreferences.bitrateKbps

                    stepSize: 500
                    from : 500
                    to: 150000

                    snapMode: "SnapOnRelease"
                    width: Math.min(bitrateDesc.implicitWidth, parent.width)

                    onValueChanged: {
                        bitrateTitle.text = "Video bitrate: " + (value / 1000.0) + " Mbps"
                        StreamingPreferences.bitrateKbps = value
                    }
                }

                Label {
                    width: parent.width
                    id: windowModeTitle
                    text: qsTr("Display mode")
                    font.pointSize: 12
                    wrapMode: Text.Wrap
                }

                AutoResizingComboBox {
                    // ignore setting the index at first, and actually set it when the component is loaded
                    Component.onCompleted: {
                        // Set the recommended option based on the OS
                        for (var i = 0; i < windowModeListModel.count; i++) {
                             var thisWm = windowModeListModel.get(i).val;
                             if (thisWm === StreamingPreferences.recommendedFullScreenMode) {
                                 windowModeListModel.get(i).text += " (Recommended)"
                                 windowModeListModel.move(i, 0, 1);
                                 break
                             }
                        }

                        currentIndex = 0

                        if (SystemProperties.hasWindowManager && !SystemProperties.rendererAlwaysFullScreen) {
                            var savedWm = StreamingPreferences.windowMode
                            for (var i = 0; i < windowModeListModel.count; i++) {
                                 var thisWm = windowModeListModel.get(i).val;
                                 if (savedWm === thisWm) {
                                     currentIndex = i
                                     break
                                 }
                            }
                        }

                        activated(currentIndex)
                    }

                    id: windowModeComboBox
                    enabled: SystemProperties.hasWindowManager && !SystemProperties.rendererAlwaysFullScreen
                    hoverEnabled: true
                    textRole: "text"
                    model: ListModel {
                        id: windowModeListModel
                        ListElement {
                            text: "Full-screen"
                            val: StreamingPreferences.WM_FULLSCREEN
                        }
                        ListElement {
                            text: "Borderless windowed"
                            val: StreamingPreferences.WM_FULLSCREEN_DESKTOP
                        }
                        ListElement {
                            text: "Windowed"
                            val: StreamingPreferences.WM_WINDOWED
                        }
                    }
                    onActivated: {
                        StreamingPreferences.windowMode = windowModeListModel.get(currentIndex).val
                    }

                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000
                    ToolTip.visible: hovered
                    ToolTip.text: "Full-screen generally provides the best performance, but borderless windowed may work better with features like macOS Spaces, Alt+Tab, screenshot tools, on-screen overlays, etc."
                }

                CheckBox {
                    id: vsyncCheck
                    width: parent.width
                    hoverEnabled: true
                    text: "V-Sync"
                    font.pointSize:  12
                    visible: false
                    checked: StreamingPreferences.enableVsync
                    onCheckedChanged: {
                        StreamingPreferences.enableVsync = checked
                    }

                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000
                    ToolTip.visible: hovered
                    ToolTip.text: "Disabling V-Sync allows sub-frame rendering latency, but it can display visible tearing"
                }

                CheckBox {
                    id: framePacingCheck
                    width: parent.width
                    hoverEnabled: true
                    text: "Frame pacing"
                    font.pointSize:  12
                    visible: false
                    enabled: StreamingPreferences.enableVsync
                    checked: StreamingPreferences.enableVsync && StreamingPreferences.framePacing
                    onCheckedChanged: {
                        StreamingPreferences.framePacing = checked
                    }
                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000
                    ToolTip.visible: hovered
                    ToolTip.text: "Frame pacing reduces micro-stutter by delaying frames that come in too early"
                }
            }
        }


        GroupBox {
            id: uiSettingsGroupBox
            width: (parent.width - (parent.leftPadding + parent.rightPadding))
            padding: 12
            title: "<font color=\"skyblue\">UI Settings</font>"
            font.pointSize: 12

            Column {
                anchors.fill: parent
                spacing: 5

                CheckBox {
                    id: startMaximizedCheck
                    width: parent.width
                    text: "Maximize Moonlight window on startup"
                    font.pointSize: 12
                    enabled: SystemProperties.hasWindowManager
                    checked: !StreamingPreferences.startWindowed || !SystemProperties.hasWindowManager
                    onCheckedChanged: {
                        StreamingPreferences.startWindowed = !checked
                    }
                }

            }
        }
    }

    Column {
        padding: 10
        rightPadding: 20
        anchors.left: settingsColumn1.right
        id: settingsColumn2
        width: settingsPage.width / 2
        spacing: 15
        visible: false

        GroupBox {
            id: gamepadSettingsGroupBox
            width: (parent.width - (parent.leftPadding + parent.rightPadding))
            padding: 12
            title: "<font color=\"skyblue\">Input Settings</font>"
            font.pointSize: 12


            Column {
                anchors.fill: parent
                spacing: 5

                CheckBox {
                    id: singleControllerCheck
                    width: parent.width
                    text: "Force gamepad #1 always present"
                    font.pointSize:  12
                    checked: !StreamingPreferences.multiController
                    onCheckedChanged: {
                        StreamingPreferences.multiController = !checked
                    }

                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000
                    ToolTip.visible: hovered
                    ToolTip.text: "Forces a single gamepad to always stay connected to the host, even if no gamepads are actually connected to this PC.\n" +
                                  "Only enable this option when streaming a game that doesn't support gamepads being connected after startup."
                }

                CheckBox {
                    id: absoluteMouseCheck
                    hoverEnabled: true
                    width: parent.width
                    text: "Optimize mouse for remote desktop instead of games"
                    font.pointSize:  12
                    enabled: SystemProperties.hasWindowManager
                    checked: StreamingPreferences.absoluteMouseMode && SystemProperties.hasWindowManager
                    onCheckedChanged: {
                        StreamingPreferences.absoluteMouseMode = checked
                    }

                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000
                    ToolTip.visible: hovered
                    ToolTip.text: "This enables mouse control without capturing the client's mouse cursor. It will not work in most games.\n
                                   You can toggle this while streaming using Ctrl+Alt+Shift+M."
                }

                CheckBox {
                    id: absoluteTouchCheck
                    hoverEnabled: true
                    width: parent.width
                    text: "Use touchscreen as a trackpad"
                    font.pointSize:  12
                    checked: !StreamingPreferences.absoluteTouchMode
                    onCheckedChanged: {
                        StreamingPreferences.absoluteTouchMode = !checked
                    }

                    ToolTip.delay: 1000
                    ToolTip.timeout: 5000
                    ToolTip.visible: hovered
                    ToolTip.text: "When checked, the touchscreen acts like a trackpad. When unchecked, the touchscreen will directly control the mouse pointer."
                }

                CheckBox {
                    id: gamepadMouseCheck
                    hoverEnabled: true
                    width: parent.width
                    text: "Gamepad mouse mode support"
                    font.pointSize:  12
                    checked: StreamingPreferences.gamepadMouse
                    onCheckedChanged: {
                        StreamingPreferences.gamepadMouse = checked
                    }

                    ToolTip.delay: 1000
                    ToolTip.timeout: 3000
                    ToolTip.visible: hovered
                    ToolTip.text: "When enabled, holding the Start button will toggle mouse mode"
                }
            }
        }

        GroupBox {
            id: hostSettingsGroupBox
            width: (parent.width - (parent.leftPadding + parent.rightPadding))
            padding: 12
            title: "<font color=\"skyblue\">Host Settings</font>"
            font.pointSize: 12

            Column {
                anchors.fill: parent
                spacing: 5

                CheckBox {
                    id: optimizeGameSettingsCheck
                    width: parent.width
                    text: "Optimize game settings"
                    font.pointSize:  12
                    checked: StreamingPreferences.gameOptimizations
                    onCheckedChanged: {
                        StreamingPreferences.gameOptimizations = checked
                    }
                }

                CheckBox {
                    id: audioPcCheck
                    width: parent.width
                    text: "Play audio on host PC"
                    font.pointSize:  12
                    checked: StreamingPreferences.playAudioOnHost
                    onCheckedChanged: {
                        StreamingPreferences.playAudioOnHost = checked
                    }
                }
            }
        }

        GroupBox {
            id: advancedSettingsGroupBox
            width: (parent.width - (parent.leftPadding + parent.rightPadding))
            padding: 12
            title: "<font color=\"skyblue\">Advanced Settings</font>"
            font.pointSize: 12

            Column {
                anchors.fill: parent
                spacing: 5

                Label {
                    width: parent.width
                    id: resVDSTitle
                    text: qsTr("Video decoder")
                    font.pointSize: 12
                    wrapMode: Text.Wrap
                }

                AutoResizingComboBox {
                    // ignore setting the index at first, and actually set it when the component is loaded
                    Component.onCompleted: {
                        var saved_vds = StreamingPreferences.videoDecoderSelection
                        currentIndex = 0
                        for (var i = 0; i < decoderListModel.count; i++) {
                            var el_vds = decoderListModel.get(i).val;
                            if (saved_vds === el_vds) {
                                currentIndex = i
                                break
                            }
                        }
                        activated(currentIndex)
                    }

                    id: decoderComboBox
                    textRole: "text"
                    model: ListModel {
                        id: decoderListModel
                        ListElement {
                            text: "Automatic (Recommended)"
                            val: StreamingPreferences.VDS_AUTO
                        }
                        ListElement {
                            text: "Force software decoding"
                            val: StreamingPreferences.VDS_FORCE_SOFTWARE
                        }
                        ListElement {
                            text: "Force hardware decoding"
                            val: StreamingPreferences.VDS_FORCE_HARDWARE
                        }
                    }
                    // ::onActivated must be used, as it only listens for when the index is changed by a human
                    onActivated : {
                        StreamingPreferences.videoDecoderSelection = decoderListModel.get(currentIndex).val
                    }
                }

                Label {
                    width: parent.width
                    id: resVCCTitle
                    text: qsTr("Video codec")
                    font.pointSize: 12
                    wrapMode: Text.Wrap
                }

                AutoResizingComboBox {
                    // ignore setting the index at first, and actually set it when the component is loaded
                    Component.onCompleted: {
                        var saved_vcc = StreamingPreferences.videoCodecConfig
                        currentIndex = 0
                        for(var i = 0; i < codecListModel.count; i++) {
                            var el_vcc = codecListModel.get(i).val;
                            if (saved_vcc === el_vcc) {
                                currentIndex = i
                                break
                            }
                        }
                        activated(currentIndex)
                    }

                    id: codecComboBox
                    textRole: "text"
                    model: ListModel {
                        id: codecListModel
                        ListElement {
                            text: "Automatic (Recommended)"
                            val: StreamingPreferences.VCC_AUTO
                        }
                        ListElement {
                            text: "H.264"
                            val: StreamingPreferences.VCC_FORCE_H264
                        }
                        ListElement {
                            text: "HEVC (H.265)"
                            val: StreamingPreferences.VCC_FORCE_HEVC
                        }
                        ListElement {
                            text: "HEVC HDR (Experimental)"
                            val: StreamingPreferences.VCC_FORCE_HEVC_HDR
                        }
                    }
                    // ::onActivated must be used, as it only listens for when the index is changed by a human
                    onActivated : {
                        StreamingPreferences.videoCodecConfig = codecListModel.get(currentIndex).val
                    }
                }

                CheckBox {
                    id: unlockUnsupportedFps
                    width: parent.width
                    text: "Unlock unsupported FPS options"
                    font.pointSize: 12
                    checked: StreamingPreferences.unsupportedFps
                    onCheckedChanged: {
                        // This is called on init, so only do the work if we've
                        // actually changed the value.
                        if (StreamingPreferences.unsupportedFps != checked) {
                            StreamingPreferences.unsupportedFps = checked

                            // The selectable FPS values depend on whether
                            // this option is enabled or not
                            fpsComboBox.reinitialize()
                        }
                    }
                }

                CheckBox {
                    id: enableMdns
                    width: parent.width
                    text: "Automatically find PCs on the local network (Recommended)"
                    font.pointSize: 12
                    checked: StreamingPreferences.enableMdns
                    onCheckedChanged: {
                        // This is called on init, so only do the work if we've
                        // actually changed the value.
                        if (StreamingPreferences.enableMdns != checked) {
                            StreamingPreferences.enableMdns = checked

                            // We must save the updated preference to ensure
                            // ComputerManager can observe the change internally.
                            StreamingPreferences.save()

                            // Restart polling so the mDNS change takes effect
                            if (window.pollingActive) {
                                ComputerManager.stopPollingAsync()
                                ComputerManager.startPolling()
                            }
                        }
                    }
                }

                CheckBox {
                    id: quitAppAfter
                    width: parent.width
                    text: "Quit app after quitting session"
                    font.pointSize: 12
                    checked: StreamingPreferences.quitAppAfter
                    onCheckedChanged: {
                        StreamingPreferences.quitAppAfter = checked
                    }
                }
            }
        }
    }
}
