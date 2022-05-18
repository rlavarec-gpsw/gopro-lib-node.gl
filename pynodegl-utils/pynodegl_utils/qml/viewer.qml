/*
 * Copyright 2022 GoPro Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Templates as T
import Qt.labs.qmlmodels // DelegateChooser is experimental
import Qt.labs.platform // ColorDialog is experimental

ApplicationWindow {
    visible: true
    minimumWidth: 640
    minimumHeight: 480

    SplitView {
        anchors.fill: parent
        anchors.margins: 5

        ScrollView {
            ScrollBar.horizontal.interactive: true
            ScrollBar.vertical.interactive: true
            ColumnLayout {
                Frame {
                    ColumnLayout {
                        ComboBox {
                            model: ["16:9", "16:10", "4:3", "1:1"]
                        }
                        ComboBox {
                            model: ["Disabled", "2x", "4x", "8x"]
                        }
                        ComboBox {
                            model: ["60 FPS", "50 FPS", "25 FPS"]
                        }
                        ComboBox {
                            model: ["Verbose", "Debug", "Info"]
                        }
                        ComboBox {
                            model: ["OpenGL", "OpenGLES", "Vulkan"]
                        }
                        Button {
                            ColorDialog {
                                id: color_dialog
                                //color: model.val
                                //onRejected: currentColor = color
                                onAccepted: model.val = color
                                flags: ColorDialog.ShowAlphaChannel | ColorDialog.NoButtons // XXX no effect
                            }
                            //text: model.label
                            palette.button: color_dialog.color // currentColor
                            onClicked: color_dialog.open()
                        }
                    }
                }
                GroupBox {
                    title: "Custom Scene options"
                    //visible: ...
                }
                GroupBox {
                    title: "Scene live controls"
                    //visible:
                    ListView {
                        implicitWidth: contentItem.childrenRect.width
                        implicitHeight: contentItem.childrenRect.height
                        //Layout.fillHeight: true
                        objectName: "control_list"
                        boundsBehavior: Flickable.StopAtBounds
                        clip: true

                        delegate: DelegateChooser {
                            role: "type"
                            DelegateChoice {
                                roleValue: "range"
                                RowLayout {
                                    Text { text: model.label }
                                    Slider {
                                        from: model.min
                                        to: model.max
                                        value: model.val
                                        onMoved: model.val = value
                                    }
                                    Text { text: model.val }
                                }
                            }
                            DelegateChoice {
                                roleValue: "color"
                                Button {
                                    ColorDialog {
                                        id: color_dialog
                                        color: model.val
                                        //onRejected: currentColor = color
                                        onAccepted: model.val = color
                                        flags: ColorDialog.ShowAlphaChannel | ColorDialog.NoButtons // XXX no effect
                                    }
                                    text: model.label
                                    palette.button: color_dialog.color // currentColor
                                    onClicked: color_dialog.open()
                                }
                            }
                            DelegateChoice {
                                roleValue: "bool"
                                Switch {
                                    checked: model.val
                                    onToggled: model.val = checked
                                    text: model.label
                                }
                            }
                            DelegateChoice {
                                roleValue: "text"
                                RowLayout {
                                    Text { text: model.label }
                                    TextField {
                                        text: model.val
                                        onEditingFinished: model.val = text
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }


        Player {
            objectName: "player"
            has_stop_button: true
        }
    }
}
