import QtQuick 2.9
import QtQuick.Controls 2.2

import CloudComputerModel 1.0
import ComputerManager 1.0
import SdlGamepadKeyNavigation 1.0

CenteredGridView {
    property CloudComputerModel cloudComputerModel : createModel()

    id: cloudPcGrid
    focus: true
    activeFocusOnTab: true
    topMargin: 20
    bottomMargin: 5
    cellWidth: 310; cellHeight: 330;
    objectName: "Your Computers"

    property var computerList: []
    property bool contactingWithBackend: false

    Component.onCompleted: {
        // Don't show any highlighted item until interacting with them.
        // We do this here instead of onActivated to avoid losing the user's
        // selection when backing out of a different page of the app.
        currentIndex = -1
    }

    StackView.onActivated: {
        // This is a bit of a hack to do this here as opposed to main.qml, but
        // we need it enabled before calling getConnectedGamepads() and PcView
        // is never destroyed, so it should be okay.
        SdlGamepadKeyNavigation.enable()

        // Highlight the first item if a gamepad is connected
        if (currentIndex == -1 && SdlGamepadKeyNavigation.getConnectedGamepads() > 0) {
            currentIndex = 0
        }

        if (launcher.isLoginSuccess()) {
            launcher.performingResetMachine.connect(onPerformingResetMachine)
            launcher.resetMachineDone.connect(onResetMachineDone)

            launcher.performingGetMachineStatus.connect(onPerformingGetMachineStatus)
            launcher.getMachineStatusDone.connect(onGetMachineStatusDone)

            launcher.searchingComputerDone.connect(onSearchingComputerDone)
            launcher.computerReady.connect(onComputerReady)
        }
    }


    StackView.onDeactivating: {
        launcher.performingResetMachine.disconnect(onPerformingResetMachine)
        launcher.resetMachineDone.disconnect(onResetMachineDone)

        launcher.performingGetMachineStatus.disconnect(onPerformingGetMachineStatus)
        launcher.getMachineStatusDone.disconnect(onGetMachineStatusDone)

        launcher.searchingComputerDone.disconnect(onSearchingComputerDone)
        launcher.computerReady.disconnect(onComputerReady)
    }

    function onComputerReady(enabled, index, appIndex, appName, session){
        contactingWithBackend = false
        if (enabled)
        {
            stageLabel.text = ""
            password.text = ""
            toolBar.visible = true
            loginRequired = true

            console.log("Enabled:", enabled, "index:", index, "appIndex:", appIndex, "appName:", appName)


            var component = Qt.createComponent("StreamSegue.qml")
            var segue = component.createObject(stackView, {"appName": appName, "session": session})
            stackView.push(segue)

        }else{
            stageLabel.text = "Error in pairing with the computer..."
            loginRequired = true
            animateOpacityUp.start()

            cannotConnectWithComputerDialog.open()
        }
    }

    function onPerformingResetMachine() {
        contactingWithBackend = true
        stageLabel.text = "Performing Reboot"
    }

    function onResetMachineDone(ok) {
        contactingWithBackend = false
        console.log("Reset Machine done", ok)

        if (ok) {
            resetConfirmedDialog.open()
        }else{
            errorResetDialog.open()
        }
    }

    function onPerformingGetMachineStatus() {
        contactingWithBackend = true
        stageLabel.text = "Get machine status..."
    }

    function onGetMachineStatusDone(ok, status, desc) {

        if (ok) {
            if (status === 2)
            {
                stageLabel.text = "Pairing with the machine..."
                launcher.seekComputer(ComputerManager,
                                  myId,
                                  myCred,
                                  myKey,
                                  myServerIp,
                                  myServerName, myServerUuid, myServerCert)
            }else{
                contactingWithBackend = false
                computerStatusNotReadyDialog.status = desc
                computerStatusNotReadyDialog.open()
            }
        }else{
            contactingWithBackend = false
            errorGetComputerStatusDialog.open()
        }
    }

    function createModel()
    {
        var model = Qt.createQmlObject('import CloudComputerModel 1.0; CloudComputerModel {}', parent, '')
        model.initialize()
        for (var i=0; i<computerList.length ; i++)
        {
            model.addComputer(computerList[i].userId,
                              computerList[i].userCred,
                              computerList[i].userKey,
                              computerList[i].serverIp,
                              computerList[i].serverName,
                              computerList[i].serverUuid,
                              computerList[i].serverCert)
        }
        return model
    }


    Column {
        anchors.centerIn: parent
        spacing: 5
        visible: contactingWithBackend

        BusyIndicator {
            id: stageSpinner
            enabled: contactingWithBackend
            visible: contactingWithBackend
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Label {
            id: stageLabel
            height: stageSpinner.height
            font.pointSize: 12
            verticalAlignment: Text.AlignVCenter
            anchors.horizontalCenter: parent.horizontalCenter
            wrapMode: Text.Wrap
        }
    }

    model: cloudComputerModel

    delegate: NavigableItemDelegate {
        width: 260; height: 320;
        grid: cloudPcGrid

        Image {
            property bool isPlaceholder: false

            id: appIcon
            anchors.horizontalCenter: parent.horizontalCenter
            y: 25
            source: "qrc:/res/no_app_image.png"

            onSourceSizeChanged: {
                isPlaceholder = true

                width = 200
                height = 267
            }

            // Display a tooltip with the full name if it's truncated
            ToolTip.text: model.name
            ToolTip.delay: 1000
            ToolTip.timeout: 5000
            ToolTip.visible: (parent.hovered || parent.highlighted) && (!appNameText.visible || appNameText.truncated)
        }

        ToolButton {
            id: resumeButton
            anchors.horizontalCenterOffset: appIcon.isPlaceholder ? -47 : 0
            anchors.verticalCenterOffset: appIcon.isPlaceholder ? -75 : -60
            anchors.centerIn: appIcon
            visible: contactingWithBackend  == false
            implicitWidth: 125
            implicitHeight: 125

            Image {
                source: "qrc:/res/baseline-play_circle_filled_white-48px.svg"
                anchors.centerIn: parent
                sourceSize {
                    width: 75
                    height: 75
                }
            }

            onClicked: {
                launchSelectedMachine()
            }

            ToolTip.text: "Resume Machine"
            ToolTip.delay: 1000
            ToolTip.timeout: 3000
            ToolTip.visible: hovered
        }

        ToolButton {
            id: resetButton
            anchors.horizontalCenterOffset: appIcon.isPlaceholder ? 47 : 0
            anchors.verticalCenterOffset: appIcon.isPlaceholder ? -75 : 60
            anchors.centerIn: appIcon
            visible: contactingWithBackend  == false
            implicitWidth: 125
            implicitHeight: 125

            Image {
                source: "qrc:/res/baseline-cancel-24px.svg"
                anchors.centerIn: parent
                sourceSize {
                    width: 75
                    height: 75
                }
            }

            onClicked: {
                doRebootMachine()
            }

            ToolTip.text: "Reboot Machine"
            ToolTip.delay: 1000
            ToolTip.timeout: 3000
            ToolTip.visible: hovered
        }

        Label {
            id: appNameText
            visible: appIcon.isPlaceholder
            text: model.name
            width: appIcon.width
            height: appIcon.height
            leftPadding: 20
            rightPadding: 20
            anchors.left: appIcon.left
            anchors.right: appIcon.right
            anchors.bottom: appIcon.bottom
            font.pointSize: 22
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            elide: Text.ElideRight
        }

        function launchSelectedMachine()
        {
            if (contactingWithBackend == false)
            {
                contactingWithBackend = true

                launcher.getMachineStatus(model.name)
            }
        }

        function doRebootMachine() {
            if (contactingWithBackend == false && rebootDialog.visible == false)
            {
                rebootDialog.machineName = model.name
                rebootDialog.open()
            }
        }

        onClicked: {

        }

        onPressAndHold: {
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.RightButton;
            onClicked: {
            }
        }

        Keys.onMenuPressed: {
        }
    }

    NavigableMessageDialog {
        id: errorResetDialog
        text: "Error in rebooting the machine. Wait for a minute and try again."
    }

    NavigableMessageDialog {
        id: resetConfirmedDialog
        text: "The machine is rebooting. It will take a while"
    }

    NavigableMessageDialog {
        id: errorGetComputerStatusDialog
        text: "Error in getting the machine status. Wait for a minute and try again."
    }

    NavigableMessageDialog {
        id: computerStatusNotReadyDialog
        text: "The Machine is not ready to be connected. Current Status: ("+ status + ")\nWait for a minute and try again. You may also consider to reboot it after a while."
        property string status: ""
    }

    NavigableMessageDialog {
        id: cannotConnectWithComputerDialog
        text: "We can't connect with this Machine. Wait for a minute and try again. You may also consider to reboot it after a while."
    }


    NavigableMessageDialog {
        id: rebootDialog
        property string machineName : ""
        text:"Are you sure you want to reboot " + machineName +"? Any unsaved progress will be lost."
        standardButtons: Dialog.Yes | Dialog.No

        function rebootMachine() {
            contactingWithBackend = true
            launcher.resetMachine(machineName)
            console.log("Reboot machine " + machineName)
        }

        onAccepted: rebootMachine()
    }

    ScrollBar.vertical: ScrollBar {}
}
