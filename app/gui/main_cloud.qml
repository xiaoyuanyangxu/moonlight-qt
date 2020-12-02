import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Window 2.2

import ComputerManager 1.0
import AutoUpdateChecker 1.0
import StreamingPreferences 1.0
import SystemProperties 1.0
import SdlGamepadKeyNavigation 1.0
import QtQuick.Controls.Styles 1.4

ApplicationWindow {
    property bool pollingActive: false

    id: window
    width: 1280
    height: 600

    function onChangePassword(ok, msg) {
        console.log("onChangePassword: ", ok, msg)
        if (!ok) {
            changePasswordErrorDialog.changePasswordErrorMsg = msg
            changePasswordErrorDialog.open()
        }
    }


    background: Rectangle {
        id: background
        anchors.fill: parent;
        color: "#000000"

        Image {
            id: backgroudImage
            source: "qrc:/res/street_fighter_t.jpeg";
            fillMode: Image.PreserveAspectCrop;
            anchors.fill: parent;
            opacity: 0.3
        }
        Image {
            id: logoImage
            source: "qrc:/res/logo.png";
            fillMode: Image.PreserveAspectFit; clip:true;
            //anchors.fill: parent;
            anchors.verticalCenter: parent.verticalCenter
            x: parent.width * 0.7
            height: parent.height * 0.65;
            opacity: 1
        }

        NumberAnimation {
            id: animateLogoOpacity
            target: logoImage
            properties: "opacity"
            from: 0.1
            to: 1
            duration: 3000
            loops: Animation.Infinite
            easing {type: Easing.InOutBack; }
       }

        NumberAnimation {
            id: animateOpacity
            target: backgroudImage
            properties: "opacity"
            from: 0.3
            to: 0.
            duration: 1000
            //loops: Animation.Infinite
            easing {type: Easing.OutQuad; }
       }
        NumberAnimation {
            id: animateOpacityUp
            target: backgroudImage
            properties: "opacity"
            from: 0.0
            to: 0.3
            duration: 1000
            //loops: Animation.Infinite
            easing {type: Easing.InQuint; }
       }
    }

    visibility: (SystemProperties.hasWindowManager && StreamingPreferences.startWindowed) ? "Windowed" : "Maximized"

    StackView {
        id: stackView
        initialItem: initialView
        anchors.fill: parent
        focus: true

        onCurrentItemChanged: {
            // Ensure focus travels to the next view when going back
            //background.opacity = 0.4
            animateOpacityUp.start()
            animateLogoOpacity.start()
            if (currentItem) {
                currentItem.forceActiveFocus()
            }
        }

        Keys.onEscapePressed: {
            if (depth > 1) {
                stackView.pop()
            }
            else {
                quitConfirmationDialog.open()
            }
        }

        Keys.onBackPressed: {
            if (depth > 1) {
                stackView.pop()
            }
            else {
                quitConfirmationDialog.open()
            }
        }

        Keys.onMenuPressed: {
            settingsButton.clicked()
        }

        // This is a keypress we've reserved for letting the
        // SdlGamepadKeyNavigation object tell us to show settings
        // when Menu is consumed by a focused control.
        Keys.onHangupPressed: {
            settingsButton.clicked()
        }
    }

    // This timer keeps us polling for 5 minutes of inactivity
    // to allow the user to work with Moonlight on a second display
    // while dealing with configuration issues. This will ensure
    // machines come online even if the input focus isn't on Moonlight.
    Timer {
        id: inactivityTimer
        interval: 5 * 60000
        onTriggered: {
            if (!active && pollingActive) {
                ComputerManager.stopPollingAsync()
                pollingActive = false
            }
        }
    }

    onVisibleChanged: {
        // When we become invisible while streaming is going on,
        // stop polling immediately.
        if (!visible) {
            inactivityTimer.stop()

            if (pollingActive) {
                ComputerManager.stopPollingAsync()
                pollingActive = false
            }
        }
        else if (active) {
            // When we become visible and active again, start polling
            inactivityTimer.stop()

            // Restart polling if it was stopped
            if (!pollingActive && !notPolling) {
                //ComputerManager.startPolling()
                //pollingActive = true
            }
        }
    }

    onActiveChanged: {
        if (active) {
            // Stop the inactivity timer
            inactivityTimer.stop()

            // Restart polling if it was stopped
            if (!pollingActive && !notPolling) {
                ComputerManager.startPolling()
                pollingActive = true
            }
        }
        else {
            // Start the inactivity timer to stop polling
            // if focus does not return within a few minutes.
            inactivityTimer.restart()
        }
    }

    property bool initialized: false

    // BUG: Using onAfterSynchronizing: here causes very strange
    // failures on Linux. Many shaders fail to compile and we
    // eventually segfault deep inside the Qt OpenGL code.
    onAfterRendering: {
        // We use this callback to trigger dialog display because
        // it only happens once the window is fully constructed.
        // Doing it earlier can lead to the dialog appearing behind
        // the window or otherwise without input focus.
        if (!initialized) {
            // Set initialized before calling anything else, because
            // pumping the event loop can cause us to get another
            // onAfterRendering call and potentially reenter this code.
            initialized = true;

            if (SystemProperties.isWow64) {
               // wow64Dialog.open()
            }
            else if (!SystemProperties.hasHardwareAcceleration) {
                if (SystemProperties.isRunningXWayland) {
                    xWaylandDialog.open()
                }
                else {
                    noHwDecoderDialog.open()
                }
            }

            if (SystemProperties.unmappedGamepads) {
                unmappedGamepadDialog.unmappedGamepads = SystemProperties.unmappedGamepads
                unmappedGamepadDialog.open()
            }
        }
    }

    function navigateTo(url, objectName)
    {
        var existingItem = stackView.find(function(item, index) {
            return item.objectName === objectName
        })

        if (existingItem !== null) {
            // Pop to the existing item
            stackView.pop(existingItem)
        }
        else {
            // Create a new item
            stackView.push(url)
        }
    }

    header: ToolBar {
        id: toolBar
        height: 45

        anchors.topMargin: 5
        anchors.bottomMargin: 5

        background: Rectangle {
                color: "#ffffff"
                opacity: 0.2
        }

        Label {
            id: titleLabel
            visible: toolBar.width > 700
            anchors.fill: parent
            text: stackView.currentItem.objectName
            font.pointSize: 20
            elide: Label.ElideRight
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
        }


        RowLayout {
            spacing: 5
            anchors.leftMargin: 5
            anchors.rightMargin: 5
            anchors.fill: parent

            NavigableToolButton {
                // Only make the button visible if the user has navigated somewhere.
                visible: stackView.depth > 1 && stackView.currentItem.objectName === "SettingsCloud"
                iconSource: "qrc:/res/arrow_left.svg"
                onClicked: stackView.pop()
                Keys.onDownPressed: {
                    stackView.currentItem.forceActiveFocus(Qt.TabFocus)
                }
            }

            NavigableToolButton {
                id: settingsButton

                visible: stackView.depth > 1 && stackView.currentItem.objectName !== "SettingsCloud"

                iconSource:  "qrc:/res/settings.svg"

                onClicked: navigateTo("qrc:/gui/SettingsViewCloud.qml", "SettingsCloud")

                Keys.onDownPressed: {
                    stackView.currentItem.forceActiveFocus(Qt.TabFocus)
                }

                Shortcut {
                    id: settingsShortcut
                    sequence: StandardKey.Preferences
                    onActivated: settingsButton.clicked()
                }

                ToolTip.delay: 1000
                ToolTip.timeout: 3000
                ToolTip.visible: hovered
                ToolTip.text: "Settings" + (settingsShortcut.nativeText ? (" ("+settingsShortcut.nativeText+")") : "")
            }


            // This label will appear when the window gets too small and
            // we need to ensure the toolbar controls don't collide
            Label {
                id: titleRowLabel
                font.pointSize: titleLabel.font.pointSize
                elide: Label.ElideRight
                horizontalAlignment: Qt.AlignHCenter
                verticalAlignment: Qt.AlignVCenter
                Layout.fillWidth: true

                // We need this label to always be visible so it can occupy
                // the remaining space in the RowLayout. To "hide" it, we
                // just set the text to empty string.
                text: !titleLabel.visible ? stackView.currentItem.objectName : ""
            }

            NavigableToolButton {
                visible: stackView.depth > 1  && stackView.currentItem.objectName !== "Settings"
                id: loggoutButton
                opacity: 1.0

                iconSource:  "qrc:/res/user_white.svg"

                onClicked: {
                    menu.open()
                }

                Keys.onDownPressed: {
                    stackView.currentItem.forceActiveFocus(Qt.TabFocus)
                }

                Menu {
                    id: menu
                    MenuItem {
                        text: "Logout"
                        icon.source:  "qrc:/res/logout.svg"
                        onClicked: {
                            toolBar.visible = false
                            stackView.pop()
                            launcher.logout()
                            stackView.currentItem.forceActiveFocus(Qt.TabFocus)
                        }
                    }
                    MenuItem {
                        text: "Change Password"
                        icon.source:  "qrc:/res/password.svg"
                        onClicked: {
                            passwordChangeDialog.open()
                        }
                    }
                }
            }
        }
    }



    ErrorMessageDialog {
        id: noHwDecoderDialog
        text: "No functioning hardware accelerated H.264 video decoder was detected by Moonlight. " +
              "Your streaming performance may be severely degraded in this configuration."
        helpText: "Click the Help button for more information on solving this problem."
        helpUrl: "https://github.com/moonlight-stream/moonlight-docs/wiki/Fixing-Hardware-Decoding-Problems"
    }

    ErrorMessageDialog {
        id: xWaylandDialog
        text: "Hardware acceleration doesn't work on XWayland. Continuing on XWayland may result in poor streaming performance. " +
              "Try running with QT_QPA_PLATFORM=wayland or switch to X11."
        helpText: "Click the Help button for more information."
        helpUrl: "https://github.com/moonlight-stream/moonlight-docs/wiki/Fixing-Hardware-Decoding-Problems"
    }

    NavigableMessageDialog {
        id: wow64Dialog
        standardButtons: Dialog.Ok | Dialog.Cancel
        text: "This PC is running a 64-bit version of Windows. Please download the x64 version of Moonlight for the best streaming performance."
        onAccepted: {
            Qt.openUrlExternally("https://github.com/moonlight-stream/moonlight-qt/releases");
        }
    }

    ErrorMessageDialog {
        id: unmappedGamepadDialog
        property string unmappedGamepads : ""
        text: "Moonlight detected gamepads without a mapping:\n" + unmappedGamepads
        helpTextSeparator: "\n\n"
        helpText: "Click the Help button for information on how to map your gamepads."
        helpUrl: "https://github.com/moonlight-stream/moonlight-docs/wiki/Gamepad-Mapping"
    }

    // This dialog appears when quitting via keyboard or gamepad button
    NavigableMessageDialog {
        id: quitConfirmationDialog
        standardButtons: Dialog.Yes | Dialog.No
        text: "Are you sure you want to quit?"
        // For keyboard/gamepad navigation
        onAccepted: Qt.quit()
    }

    NavigableMessageDialog {
        id: changePasswordErrorDialog
        property string changePasswordErrorMsg: ""
        text: "Error in changing password\n" + "Error: " + changePasswordErrorMsg
    }

    // HACK: This belongs in StreamSegue but keeping a dialog around after the parent
    // dies can trigger bugs in Qt 5.12 that cause the app to crash. For now, we will
    // host this dialog in a QML component that is never destroyed.
    //
    // To repro: Start a stream, cut the network connection to trigger the "Connection
    // terminated" dialog, wait until the app grid times out back to the PC grid, then
    // try to dismiss the dialog.
    ErrorMessageDialog {
        id: streamSegueErrorDialog

        property bool quitAfter: false

        onClosed: {
            if (quitAfter) {
                Qt.quit()
            }

            // StreamSegue assumes its dialog will be re-created each time we
            // start streaming, so fake it by wiping out the text each time.
            text = ""
        }
    }
    PasswordChangeDialog {
        id: passwordChangeDialog


        onAccepted:
        {
            launcher.changePasswordDone.connect(onChangePassword)
            launcher.changePassword(launcher.getLastUsername(), oldPassword, newPassword)
        }

        onRejected: {

        }
    }
}
