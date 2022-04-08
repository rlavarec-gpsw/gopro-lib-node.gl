#
# Copyright 2022 GoPro Inc.
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#

import importlib
import os.path as op
import sys

from pynodegl_utils import qml
from pynodegl_utils.com import load_script
from PySide6.QtCore import QAbstractListModel, QModelIndex, QObject, Qt, Slot


class _Viewer:
    def __init__(self, qml_engine, ngl_widget, args):
        if len(args) != 3:
            print(f"Usage: {args[0]} [<module>|<script>] <function>")
            sys.exit(1)

        app_window = qml_engine.rootObjects()[0]
        self._livectls_model = UIElementsModel()
        list_view = app_window.findChild(QObject, "control_list")
        list_view.setProperty("model", self._livectls_model)

        self._ngl_widget = ngl_widget
        self._ngl_widget.livectls_changed.connect(self._livectls_model.reset_data_model)
        self._livectls_model.dataChanged.connect(self._livectl_data_changed)

        mod_name, func_name = args[1], args[2]
        if mod_name.endswith(".py"):
            mod = load_script(mod_name)
        else:
            mod = importlib.import_module(mod_name)
        scene_info = getattr(mod, func_name)()
        self._ngl_widget.set_scene(scene_info["scene"])

        self._player = app_window.findChild(QObject, "player")
        self._player.setProperty("duration", scene_info["duration"])
        self._player.setProperty("framerate", list(scene_info["framerate"]))
        self._player.setProperty("aspect", list(scene_info["aspect_ratio"]))
        self._player.timeChanged.connect(ngl_widget.set_time)
        self._player.stopped.connect(ngl_widget.stop)

    @Slot()
    def _livectl_data_changed(self, top_left, bottom_right):
        for row in range(top_left.row(), bottom_right.row() + 1):
            data = self._livectls_model.get_row(row)
            self._ngl_widget.livectls_changes[data["label"]] = data
        self._ngl_widget.update()


class UIElementsModel(QAbstractListModel):

    _roles_map = {Qt.UserRole + i: s for i, s in enumerate(("type", "label", "val", "min", "max"))}

    def __init__(self):
        super().__init__()
        self._data = []

    @Slot(object)
    def reset_data_model(self, data):
        self.beginResetModel()
        self._data = data
        self.endResetModel()

    def get_row(self, row):
        return self._data[row]

    def roleNames(self):
        names = super().roleNames()
        names.update({k: v.encode() for k, v in self._roles_map.items()})
        return names

    def rowCount(self, parent=QModelIndex()):
        return len(self._data)

    def data(self, index, role: int):
        if not index.isValid():
            return None
        return self._data[index.row()].get(self._roles_map[role])

    def setData(self, index, value, role):
        if not index.isValid():
            return False
        item = self._data[index.row()]
        item[self._roles_map[role]] = value
        self.dataChanged.emit(index, index)
        return True


def run():
    qml_file = op.join(op.dirname(qml.__file__), "viewer.qml")
    app, engine = qml.create_app_engine(sys.argv, qml_file)
    ngl_widget = qml.create_ngl_widget(engine)
    viewer = _Viewer(engine, ngl_widget, sys.argv)  # noqa: need to assign for correct lifetime
    sys.exit(app.exec())
