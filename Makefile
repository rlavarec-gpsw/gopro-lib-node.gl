#
# Copyright 2016 GoPro Inc.
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

# Currently we don't support custom prefix
PREFIX = venv
PREFIX_DONE     = .venv-done
export TARGET_OS ?= $(shell uname -s)
ifeq ($(TARGET_OS),Windows)
PREFIX_FULLPATH = $(shell wslpath -wa .)\$(PREFIX)
else
PREFIX_FULLPATH = $(PWD)/$(PREFIX)
endif

PYTHON_MAJOR = 3

DEBUG      ?= no
COVERAGE   ?= no
ifeq ($(TARGET_OS),Windows)
PYTHON     ?= python.exe
else
PYTHON     ?= python$(if $(shell which python$(PYTHON_MAJOR) 2> /dev/null),$(PYTHON_MAJOR),)
endif

ifeq ($(TARGET_OS),Windows)
PIP = $(PREFIX)/Scripts/pip.exe
else
PIP = $(PREFIX)/bin/pip
endif
TARGET_OS_LOWERCASE = $(shell $(PYTHON) -c "print('$(TARGET_OS)'.lower())" )
DEBUG_GL    ?= no
DEBUG_MEM   ?= no
DEBUG_SCENE ?= no
export DEBUG_GPU_CAPTURE ?= no
TESTS_SUITE ?=
V           ?=

ifeq ($(TARGET_OS),Windows)
# Initialize VCVARS64 and VCPKG_DIR to a default value
# Note: the user should override this environment variable if needed
VCVARS64 ?= "$(shell powershell.exe .\\scripts\\find_vcvars64.ps1)"
VCPKG_DIR ?= C:\\vcpkg
PKG_CONF_DIR = external\\pkgconf\\build
# General way to call cmd from bash: https://github.com/microsoft/WSL/issues/2835
# Add the character @ after /C
CMD = cmd.exe /C @
else
CMD =
endif

ifneq ($(shell $(CMD) $(PYTHON) -c "import sys;print(sys.version_info.major)"),$(PYTHON_MAJOR))
$(error "Python $(PYTHON_MAJOR) not found")
endif

ifeq ($(TARGET_OS),Windows)
ACTIVATE = "$(PREFIX_FULLPATH)\\Scripts\\activate.bat"
else
ACTIVATE = $(PREFIX_FULLPATH)/bin/activate
endif

RPATH_LDFLAGS = -Wl,-rpath,$(PREFIX_FULLPATH)/lib

ifeq ($(TARGET_OS),Windows)
MESON = meson.exe
MESON_SETUP_PARAMS  = \
    --prefix="$(PREFIX_FULLPATH)" --bindir="$(PREFIX_FULLPATH)\\Scripts" --includedir="$(PREFIX_FULLPATH)\\Include" \
    --libdir="$(PREFIX_FULLPATH)\\Lib" --pkg-config-path="$(VCPKG_DIR)\\installed\x64-$(TARGET_OS_LOWERCASE)\\lib\\pkgconfig;$(PREFIX_FULLPATH)\\Lib\\pkgconfig"
else
MESON = meson
MESON_SETUP_PARAMS = --prefix=$(PREFIX_FULLPATH) --pkg-config-path=$(PREFIX_FULLPATH)/lib/pkgconfig -Drpath=true
endif
MESON_SETUP         = $(MESON) setup --backend $(MESON_BACKEND) $(MESON_SETUP_PARAMS)
MESON_TEST          = $(MESON) test

MESON_COMPILE = MAKEFLAGS= $(MESON) compile -j8
MESON_INSTALL = $(MESON) install
ifeq ($(COVERAGE),yes)
MESON_SETUP += -Db_coverage=true
DEBUG = yes
endif
ifeq ($(DEBUG),yes)
MESON_SETUP += --buildtype=debugoptimized
else
MESON_SETUP += --buildtype=release
ifneq ($(TARGET_OS),MinGW-w64)
MESON_SETUP += -Db_lto=true
endif
endif
ifneq ($(V),)
MESON_COMPILE += -v
endif
NODEGL_DEBUG_OPTS-$(DEBUG_GL)    += gl
NODEGL_DEBUG_OPTS-$(DEBUG_MEM)   += mem
NODEGL_DEBUG_OPTS-$(DEBUG_SCENE) += scene
NODEGL_DEBUG_OPTS-$(DEBUG_GPU_CAPTURE) += gpu_capture
ifneq ($(NODEGL_DEBUG_OPTS-yes),)
NODEGL_DEBUG_OPTS = -Ddebug_opts=$(shell echo $(NODEGL_DEBUG_OPTS-yes) | tr ' ' ',')
endif
ifeq ($(DEBUG_GPU_CAPTURE),yes)
ifeq ($(TARGET_OS),Windows)
RENDERDOC_DIR = $(shell wslpath -wa .)\external\renderdoc
else
RENDERDOC_DIR = $(PWD)/external/renderdoc
endif
NODEGL_DEBUG_OPTS += -Drenderdoc_dir="$(RENDERDOC_DIR)"
endif

# Workaround Debian/Ubuntu bug; see https://github.com/mesonbuild/meson/issues/5925
ifeq ($(TARGET_OS),Linux)
DISTRIB_ID := $(or $(shell lsb_release -si 2>/dev/null),none)
ifeq ($(DISTRIB_ID),$(filter $(DISTRIB_ID),Ubuntu Debian))
MESON_SETUP += --libdir lib
endif
endif

ifneq ($(TESTS_SUITE),)
MESON_TESTS_SUITE_OPTS += --suite $(TESTS_SUITE)
endif

NODEGL_SETUP_OPTS =

ifeq ($(TARGET_OS),Windows)
MESON_BACKEND ?= vs
else ifeq ($(TARGET_OS),Darwin)
MESON_BACKEND ?= xcode
else
MESON_BACKEND ?= ninja
endif

MESON_BUILDDIR ?= builddir

all: ngl-tools-install pynodegl-utils-install
	@echo
	@echo "    Install completed."
	@echo
	@echo "    You can now enter the venv with:"
ifeq ($(TARGET_OS),Windows)
	@echo "        (via Windows Command Prompt)"
	@echo "            cmd.exe"
	@echo "            $(PREFIX)\\\Scripts\\\activate.bat"
	@echo "        (via Windows PowerShell)"
	@echo "            powershell.exe"
	@echo "            $(PREFIX)\\\Scripts\\\Activate.ps1"
else
	@echo "        $(ACTIVATE)"
endif
	@echo

ngl-tools-install: nodegl-install
	$(MESON_SETUP) --backend $(MESON_BACKEND) ngl-tools $(MESON_BUILDDIR)/ngl-tools
	$(MESON_COMPILE) -C $(MESON_BUILDDIR)/ngl-tools && $(MESON_INSTALL) -C $(MESON_BUILDDIR)/ngl-tools

pynodegl-utils-install: pynodegl-utils-deps-install
	($(PIP) install -e ./pynodegl-utils)

#
# pynodegl-install is in dependency to prevent from trying to install pynodegl
# from its requirements. Pulling pynodegl from requirements has two main issue:
# it tries to get it from PyPi (and we want to install the local pynodegl
# version), and it would fail anyway because pynodegl is currently not
# available on PyPi.
#
# We do not pull the requirements on Windows because of various issues:
# - PySide2 can't be pulled (required to be installed by the user outside the
#   Python virtualenv)
# - Pillow fails to find zlib (required to be installed by the user outside the
#   Python virtualenv)
# - ngl-control can not currently work because of:
#     - temporary files handling
#     - subprocess usage, passing fd is not supported on Windows
#     - subprocess usage, Windows cannot execute directly hooks shell scripts
#
# Still, we want the module to be installed so we can access the scene()
# decorator and other related utils.
#
pynodegl-utils-deps-install: pynodegl-install
	($(PIP) install -r ./pynodegl-utils/requirements.txt)

pynodegl-install: pynodegl-deps-install
	($(PIP) -v install -e ./pynodegl)

pynodegl-deps-install: $(PREFIX_DONE) nodegl-install
	($(PIP) install -r ./pynodegl/requirements.txt)

nodegl-install: nodegl-setup
	($(MESON_COMPILE) -C $(MESON_BUILDDIR)/libnodegl && $(MESON_INSTALL) -C $(MESON_BUILDDIR)/libnodegl)

NODEGL_DEPS=sxplayer-install
nodegl-setup: $(NODEGL_DEPS)
	($(MESON_SETUP) --backend $(MESON_BACKEND) $(NODEGL_SETUP_OPTS) $(NODEGL_DEBUG_OPTS) --default-library shared libnodegl $(MESON_BUILDDIR)/libnodegl)

pkg-config-install: external-download $(PREFIX_DONE)
ifeq ($(TARGET_OS),Windows)
	($(MESON_SETUP) -Dtests=false external/pkgconf $(MESON_BUILDDIR)/pkgconf && $(MESON_COMPILE) -C builddir/pkgconf && $(MESON_INSTALL) -C $(MESON_BUILDDIR)/pkgconf)
endif

sxplayer-install: external-download pkg-config-install $(PREFIX_DONE)
	($(MESON_SETUP) external/sxplayer $(MESON_BUILDDIR)/sxplayer && $(MESON_COMPILE) -C $(MESON_BUILDDIR)/sxplayer && $(MESON_INSTALL) -C $(MESON_BUILDDIR)/sxplayer)

external-download:
	$(MAKE) -C external

#
# We do not pull meson from pip on MinGW for the same reasons we don't pull
# Pillow and PySide2. We require the users to have it on their system.
#
$(PREFIX_DONE):
ifeq ($(TARGET_OS),Windows)
	($(PYTHON) -m venv $(PREFIX))
	(mkdir $(PREFIX)/Lib/pkgconfig)
else ifeq ($(TARGET_OS),MinGW-w64)
	$(PYTHON) -m venv --system-site-packages  $(PREFIX)
else
	$(PYTHON) -m venv $(PREFIX)
	($(ACTIVATE) && pip install meson ninja)
endif
	touch $(PREFIX_DONE)

$(PREFIX): $(PREFIX_DONE)

tests: nodegl-tests tests-setup
	($(MESON) test $(MESON_TESTS_SUITE_OPTS) -C $(MESON_BUILDDIR)/tests)

tests-setup: ngl-tools-install pynodegl-utils-install
	$(MESON_SETUP) --backend ninja $(MESON_BUILDDIR)/tests tests

nodegl-tests: nodegl-install
	$(MESON_TEST) -C $(MESON_BUILDDIR)/libnodegl

compile-%:
	$(MESON_COMPILE) -C $(MESON_BUILDDIR)/$(subst compile-,,$@)

install-%: compile-%
	$(MESON_INSTALL) -C $(MESON_BUILDDIR)/$(subst install-,,$@)

nodegl-%: nodegl-setup
	$(MESON_COMPILE) -C $(MESON_BUILDDIR)/libnodegl $(subst nodegl-,,$@)

clean_py:
	$(RM) pynodegl/nodes_def.pyx
	$(RM) pynodegl/pynodegl.c
	$(RM) pynodegl/pynodegl.*.so
	$(RM) -r pynodegl/build
	$(RM) -r pynodegl/pynodegl.egg-info
	$(RM) -r pynodegl/.eggs
	$(RM) -r pynodegl-utils/pynodegl_utils.egg-info
	$(RM) -r pynodegl-utils/.eggs

clean: clean_py
	$(RM) -r $(MESON_BUILDDIR)/sxplayer
	$(RM) -r $(MESON_BUILDDIR)/libnodegl
	$(RM) -r $(MESON_BUILDDIR)/ngl-tools
	$(RM) -r $(MESON_BUILDDIR)/tests

# You need to build and run with COVERAGE set to generate data.
# For example: `make clean && make -j8 tests COVERAGE=yes`
# We don't use `meson coverage` here because of
# https://github.com/mesonbuild/meson/issues/7895
coverage-html:
	(ninja -C $(MESON_BUILDDIR)/libnodegl coverage-html)
coverage-xml:
	(ninja -C $(MESON_BUILDDIR)/libnodegl coverage-xml)

.PHONY: all
.PHONY: ngl-tools-install
.PHONY: pynodegl-utils-install pynodegl-utils-deps-install
.PHONY: pynodegl-install pynodegl-deps-install
.PHONY: nodegl-install nodegl-setup
.PHONY: sxplayer-install
.PHONY: tests tests-setup
.PHONY: clean clean_py
.PHONY: coverage-html coverage-xml
.PHONY: external-download

#
