#!/usr/bin/env python

import sys
import os
import os.path as op
from PySide6.QtGui import QGuiApplication, QOpenGLFunctions
from PySide6.QtOpenGL import QOpenGLShader, QOpenGLShaderProgram #, QOpenGLVersionProfile
from PySide6.QtQuick import QQuickView, QQuickItem, QQuickWindow, QSGRendererInterface

from PySide6 import QtGui
from PySide6 import QtCore
from PySide6.QtQml import qmlRegisterType

from pynodegl_utils.examples.misc import fibo, cube

import pynodegl as ngl

import OpenGL.GL as GL


class Squircle(QQuickItem):

    tChanged = QtCore.Signal()

    def __init__(self):
        super().__init__()
        self._t = 0.0
        self._renderer = None
        self.windowChanged.connect(self.handleWindowChanged)

    def t(self):
        return self._t

    def setT(self, t):
        if self._t == t:
            return
        self._t = t
        self.tChanged.emit()
        if self.window():
            self.window().update()

    t = QtCore.Property(float, t, setT, notify=tChanged)

    @QtCore.Slot(QQuickWindow)
    def handleWindowChanged(self, win):
        if win:
            win.beforeSynchronizing.connect(self.sync, QtCore.Qt.DirectConnection)
            win.sceneGraphInvalidated.connect(self.cleanup, QtCore.Qt.DirectConnection)
            # Ensure we start with cleared to black. The squircle's blend mode relies on this.
            win.setColor(QtCore.Qt.black)
            self.sync()

    @QtCore.Slot()
    def cleanup(self):
        del self._renderer
        self._renderer = None

    def releaseResources(self):
        self.window().scheduleRenderJob(CleanupJob(self._renderer), QQuickWindow.BeforeSynchronizingStage)
        self._renderer = None

    @QtCore.Slot()
    def sync(self):
        if not self._renderer:
            self._renderer = SquircleRenderer()
            self.window().beforeRendering.connect(self._renderer.init, QtCore.Qt.DirectConnection)
            self.window().beforeRenderPassRecording.connect(self._renderer.paint, QtCore.Qt.DirectConnection)
        self._renderer.setViewportSize(self.window().size() * self.window().devicePixelRatio())
        self._renderer.setT(self._t)
        self._renderer.setWindow(self.window())


class CleanupJob(QtCore.QRunnable):

    def __init__(self, renderer):
        super().__init__()
        self._renderer = renderer

    def run(self):
        del self._renderer


class SquircleRenderer(QOpenGLFunctions):

    def __init__(self):
        super().__init__()
        self._t = 0.0
        self._context = None

    def setT(self, t):
        self._t = t

    def setViewportSize(self, size):
        self._viewportSize = size

    def setWindow(self, window):
        self._window = window

    @QtCore.Slot()
    def init(self):
        if not self._context:
            rif = self._window.rendererInterface()
            assert rif.graphicsApi() == QSGRendererInterface.OpenGL or rif.graphicsApi() == QSGRendererInterface.OpenGLRhi
            self._context = ngl.Context()
            GL.glGetError()
            self._context.configure(wrapped=1, backend=ngl.BACKEND_OPENGL)
            assert self._context.set_scene(fibo().get('scene')) == 0


    @QtCore.Slot()
    def paint(self):
        # Play nice with the RHI. Not strictly needed when the scenegraph uses
        # OpenGL directly.
        self._window.beginExternalCommands()

        self._context.resize(self._viewportSize.width(), self._viewportSize.height(), None)
        self._context.draw(self._t)

        self._window.endExternalCommands()


def get_surface_format(backend='gl', version=None):
    surface_format = QtGui.QSurfaceFormat()

    if backend == 'gl':
        surface_format.setRenderableType(QtGui.QSurfaceFormat.OpenGL)
        major_version, minor_version = (4, 1) if version is None else version
        surface_format.setVersion(major_version, minor_version)
        surface_format.setProfile(QtGui.QSurfaceFormat.CoreProfile)
    elif backend == 'gles':
        surface_format.setRenderableType(QtGui.QSurfaceFormat.OpenGLES)
        major_version, minor_version = (2, 0) if version is None else version
        surface_format.setVersion(major_version, minor_version)
    else:
        raise Exception('Unknown rendering backend' % backend)

    surface_format.setDepthBufferSize(24)
    surface_format.setStencilBufferSize(8)
    surface_format.setAlphaBufferSize(8)
    return surface_format


if __name__ == '__main__':
    os.environ['QT_XCB_GL_INTEGRATION'] = 'xcb_egl'
    QtGui.QSurfaceFormat.setDefaultFormat(get_surface_format('gl'))
    app = QGuiApplication(sys.argv)
    QQuickWindow.setGraphicsApi(QSGRendererInterface.OpenGL)
    qmlRegisterType(Squircle, 'OpenGLUnderQML', 1, 0, 'Squircle')
    view = QQuickView()
    view.setResizeMode(QQuickView.SizeRootObjectToView)
    view.setSource(op.join(op.dirname(__file__), "main6.qml"))
    view.show()
    sys.exit(app.exec())
