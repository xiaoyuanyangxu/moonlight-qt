import QtQuick 2.0
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.2

NavigableDialog {
    id: dialog
    width: 300

    property string oldPassword
    property string newPassword

    onOpened: {
        oldPass.text = ""
        newPass.text = ""
        newPass2.text = ""
    }

    Column {
        spacing: 5

        TextField {
            width: 250
            id: oldPass
            placeholderText: "Enter your old password"
            echoMode: TextInput.Password
            focus: true
        }
        TextField {
            width: 250
            id: newPass
            placeholderText: "Enter your new password"
            echoMode: TextInput.Password
        }
        TextField {
            width: 250
            id: newPass2
            placeholderText: "Please repeat"
            echoMode: TextInput.Password
        }
        Row {
            width: parent.width
            spacing: 2
            Button {
                width: (parent.width -1) / 2
                text: "Cancel"
                onClicked: reject()
            }
            Button {
                width: (parent.width - 1) / 2
                text: "<font color='#00ff00'>Change</font>"

                enabled: oldPass.displayText !== "" && newPass.displayText !== "" && newPass2.displayText !== "" && newPass.text === newPass2.text
                onClicked: {
                    oldPassword = oldPass.text
                    newPassword = newPass.text
                    accept()
                }
            }
        }
    }
}
