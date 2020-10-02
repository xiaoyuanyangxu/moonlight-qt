import QtQuick 2.0
import QtQuick.Controls 2.2

import ComputerManager 1.0
import StreamingPreferences 1.0
import SdlGamepadKeyNavigation 1.0

Item {
    property bool loginRequired: false
    property bool contactingWithBackend: false
    property string sessionId: "i_am_ncool"
    property string myId: ""
    property string myCred: ""
    property string myKey: ""
    property string myServerIp: ""
    property string computerName: ""

    function onSearchingComputer() {
        contactingWithBackend = true
        stageLabel.text = "Establishing connection to PC..."
    }

    function onSearchingApp() {
        stageLabel.text = "Loading app list..."
    }

    function onPerformingGetMyCreadentials() {
        contactingWithBackend = true
        stageLabel.text = "It is getting your credentials..."
    }


    function onSearchingComputerDone(ok, data) {
        contactingWithBackend = false
        if (ok) {
            computerName = data
        }else{
            stageLabel.text = "Error in pairing with the computer..."
            loginRequired = true
            animateOpacityUp.start()
        }
    }

    function onComputerReady(enabled, index){
        contactingWithBackend = false
        if (enabled)
        {
            stageLabel.text = ""
            password.text = ""
            toolBar.visible = true
            loginRequired = true
            var component = Qt.createComponent("AppView.qml")
            var appView = component.createObject(stackView, {"computerIndex": index, "objectName": computerName})
            stackView.push(appView)

        }else{
            stageLabel.text = "Error in pairing with the computer..."
            loginRequired = true
            animateOpacityUp.start()
        }
    }

    function onMyCredentialsDone(ok, id, cred, key, ip, name, uuid, cert) {
        contactingWithBackend = false
        if (ok) {
            stageLabel.text = "We got your credentials";
            myId = id
            myCred = cred
            myKey = key
            myServerIp = ip
            loginRequired = false

            if (myId.length == 0 || myCred.length == 0 ||
                myKey.length == 0 || myServerIp.length == 0)
            {
                stageLabel.text = "No server is available. Try again!";
                loginRequired = true
                animateOpacityUp.start()

            }else{
                launcher.seekComputer(ComputerManager,
                                  myId,
                                  myCred,
                                  myKey,
                                  myServerIp, name, uuid, cert)
            }
        }else{
            stageLabel.text = "Error in getting your creadentials. Login required";
            loginRequired = true
            animateOpacityUp.start()
        }
    }

    function performLogin(username, password) {
        launcher.login(username, password)
    }

    function onPerformingLogin() {
        contactingWithBackend = true
        stageLabel.text = "Performing login..."
    }

    function onLogginDone(ok, id) {
        contactingWithBackend = false

        if (ok) {
            stageLabel.text = "Loggin successed";
            loginRequired = false
            sessionId = id
            launcher.getMyCredentials(sessionId)
        }else{
            stageLabel.text = "Loggin failed!"
            loginRequired = true
            animateOpacityUp.start()
        }
    }

    function onSessionCreated(appName, session) {
        var component = Qt.createComponent("StreamSegue.qml")
        var segue = component.createObject(stackView, {
            "appName": appName,
            "session": session,
            "quitAfter": true
        })
        stackView.push(segue)
    }

    function onLaunchFailed(message) {
        errorDialog.text = message
        errorDialog.open()
    }

    function onAppQuitRequired(appName) {
        quitAppDialog.appName = appName
        quitAppDialog.open()
    }

    StackView.onActivated: {


        SdlGamepadKeyNavigation.enable()
        if (!launcher.isExecuted()) {
            toolBar.visible = false
            launcher.searchingComputer.connect(onSearchingComputer)
            launcher.performingGetMyCreadentials.connect(onPerformingGetMyCreadentials)
            launcher.myCredentialsDone.connect(onMyCredentialsDone)
            launcher.performingLogin.connect(onPerformingLogin)
            launcher.logginDone.connect(onLogginDone)

            launcher.searchingComputerDone.connect(onSearchingComputerDone)
            launcher.computerReady.connect(onComputerReady)

            sessionId = launcher.getCachedSessionCookie();
            if (sessionId.length > 0) {
                launcher.getMyCredentials(sessionId)
            }else{
                loginRequired = true
            }
        }
    }

    Column {
        anchors.centerIn: parent
        spacing: 5

        TextField {
            id: login
            placeholderText: "Username"
            text: launcher.getLastUsername()
            color: "white"
            enabled: loginRequired && !contactingWithBackend
            visible: loginRequired
            anchors.horizontalCenter: parent.horizontalCenter
        }

        TextField {
            id: password
            placeholderText: "Password"
            echoMode: TextInput.Password
            enabled: loginRequired && !contactingWithBackend
            visible: loginRequired
            anchors.horizontalCenter: parent.horizontalCenter
        }

        Button {
            id: proccessButton

            text: " Login"
            icon.color: "transparent"
            icon.source: "qrc:/res/login.svg"
            enabled: loginRequired && login.text.length > 0 && password.text.length >0 && !contactingWithBackend
            visible: loginRequired
            onClicked: {
                animateOpacity.start()
                performLogin(login.text, password.text)
            }
            anchors.horizontalCenter: parent.horizontalCenter
        }


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

    ErrorMessageDialog {
        id: errorDialog

        onClosed: {
            Qt.quit();
        }
    }

    NavigableMessageDialog {
        id: quitAppDialog
        text:"Are you sure you want to quit " + appName +"? Any unsaved progress will be lost."
        standardButtons: Dialog.Yes | Dialog.No
        property string appName : ""

        function quitApp() {
            var component = Qt.createComponent("QuitSegue.qml")
            var params = {"appName": appName}
            stackView.push(component.createObject(stackView, params))
            // Trigger the quit after pushing the quit segue on screen
            launcher.quitRunningApp()
        }

        onAccepted: quitApp()
        onRejected: Qt.quit()
    }
}
