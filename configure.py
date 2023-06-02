#!/usr/bin/env python3
#
# Copyright 2021-2022 GoPro Inc.
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

import argparse
import glob
import hashlib
import logging
import os
import os.path as op
import pathlib
import platform
import shlex
import shutil
import stat
import sysconfig
import tarfile
import urllib.request
import venv
import zipfile
from multiprocessing import Pool, cpu_count
from subprocess import run

import certifi

_ROOTDIR = op.abspath(op.dirname(__file__))
_SYSTEM = "MinGW" if sysconfig.get_platform().startswith("mingw") else platform.system()
_RENDERDOC_ID = f"renderdoc_{_SYSTEM}"
_EXTERNAL_DEPS = dict(
    sxplayer=dict(
        version="9.13.0",
        url="https://github.com/gopro/sxplayer/archive/v@VERSION@.tar.gz",
        dst_file="sxplayer-@VERSION@.tar.gz",
        sha256="272ecf2a31238440a8fc70dbe6431989d06cf77b9f15a26a55c6c91244768fca",
    ),
    pkgconf=dict(
        version="1.8.0",
        url="https://github.com/pkgconf/pkgconf/archive/pkgconf-@VERSION@.tar.gz",
        dst_file="pkgconf-@VERSION@.tar.gz",
        sha256="d84a2a338a17e0f68e6a8d6c9caf40d6a2c9580c4ae1d475b338b8d956e483aa",
    ),
    renderdoc_Windows=dict(
        version="1.18",
        url="https://renderdoc.org/stable/@VERSION@/RenderDoc_@VERSION@_64.zip",
        sha256="a97a9911850c8a93dc1dee8f94e339cd5933310513dddf0216d27cea3a5f25b1",
    ),
    renderdoc_Linux=dict(
        version="1.18",
        url="https://renderdoc.org/stable/@VERSION@/renderdoc_@VERSION@.tar.gz",
        sha256="c8ec16f7463266641e21b64f8e436a452a15105e4bd517bf114a9349d74cc02e",
    ),
    json=dict(
        version="3.11.2",
        url="https://github.com/nlohmann/json/releases/download/v@VERSION@/json.hpp",
        dst_dir="json",
        sha256="665fa14b8af3837966949e8eb0052d583e2ac105d3438baba9951785512cf921",
    ),
    d3dx12_ShaderCompiler=dict(
        version="1.7.2212.1",
        url="https://github.com/microsoft/DirectXShaderCompiler/releases/download/v@VERSION@/dxc_2023_03_01.zip",
        uncompress_dir="d3dx12_ShaderCompiler-@VERSION@",
        dst_file="d3dx12_ShaderCompiler-@VERSION@.zip",
        sha256="e4e8cb7326ff7e8a791acda6dfb0cb78cc96309098bfdb0ab1e465dc29162422",
    ),
    d3dx12_AgilitySDK=dict(
        version="1.610.3",
        url="https://globalcdn.nuget.org/packages/microsoft.direct3d.d3d12.@VERSION@.nupkg",
        uncompress_dir="d3dx12_AgilitySDK-@VERSION@",
        dst_file="d3dx12_AgilitySDK-@VERSION@.zip",
        sha256="3e4f2905aa9baf159c3a2b87f8d1ef6e9a31177d5f66da06c019af2254787248",
    ),
    shaderc=dict(
        version="2023.3",
        url="https://github.com/google/shaderc/archive/refs/tags/v@VERSION@.zip",
        dst_file="shaderc-@VERSION@.zip",
        sha256="9609a7591362e6648e78c2a4b0adce95527d0b411ff45ff7f3e5151e2966a1d3",
    ),
)


def _get_external_deps(args):
    deps = ["sxplayer"]
    if _SYSTEM == "Windows" or _SYSTEM == "Darwin":
        deps.append("json")
        deps.append("shaderc")

    if _SYSTEM == "Windows":
        deps.append("pkgconf")
        deps.append("d3dx12_ShaderCompiler")
        deps.append("d3dx12_AgilitySDK")


    if "gpu_capture" in args.debug_opts:
        if _SYSTEM not in {"Windows", "Linux"}:
            raise Exception(f"Renderdoc is not supported on {_SYSTEM}")
        deps.append(_RENDERDOC_ID)
    return {dep: _EXTERNAL_DEPS[dep] for dep in deps}


def _guess_base_dir(dirs):
    smallest_dir = sorted(dirs, key=lambda x: len(x))[0]
    return pathlib.Path(smallest_dir).parts[0]


def _get_brew_prefix():
    prefix = None
    try:
        proc = run(["brew", "--prefix"], capture_output=True, text=True, check=True)
        prefix = proc.stdout.strip()
    except FileNotFoundError:
        # Silently pass if brew is not installed
        pass
    return prefix


def _file_chk(path, chksum_hexdigest):
    chksum = hashlib.sha256()
    with open(path, "rb") as f:
        while True:
            buf = f.read(8196)
            if not buf:
                break
            chksum.update(buf)
    match_ = chksum.hexdigest() == chksum_hexdigest
    if not match_:
        logging.warning("%s: mismatching check sum", path)
    return match_


def _fix_permissions(path):
    for root, dirs, files in os.walk(path, topdown=True):
        for file in files:
            os.chmod(op.join(root, file), stat.S_IRUSR | stat.S_IWUSR)
        for directory in dirs:
            os.chmod(op.join(root, directory), stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)


def _rmtree(path, ignore_errors=False, onerror=None):
    """
    shutil.rmtree wrapper that is resilient to permission issues when
    encountering read-only files or directories lacking the executable
    permission.
    """
    try:
        shutil.rmtree(path, ignore_errors, onerror=onerror)
    except Exception:
        _fix_permissions(path)
        shutil.rmtree(path, ignore_errors, onerror=onerror)


def _download_extract(dep_item):
    logging.basicConfig(level="INFO")  # Needed for every process on Windows

    name, dep = dep_item

    version = dep["version"]
    url = dep["url"].replace("@VERSION@", version)
    chksum = dep["sha256"]
    dst_file = dep.get("dst_file", op.basename(url)).replace("@VERSION@", version)
    dst_base = op.join(_ROOTDIR, "external")
    if "dst_dir" in dep:
        dst_base = op.join(dst_base, dep["dst_dir"])
    uncompress_dir = ''
    if "uncompress_dir" in dep:
        uncompress_dir = op.join(dst_base, dep["uncompress_dir"]).replace("@VERSION@", version)

    dst_path = op.join(dst_base, dst_file)
    os.makedirs(dst_base, exist_ok=True)

    needCreateAlink = True

    # Download
    if not op.exists(dst_path) or not _file_chk(dst_path, chksum):
        logging.info("downloading %s to %s", url, dst_file)
        urllib.request.urlretrieve(url, dst_path)
        assert _file_chk(dst_path, chksum)

    # Extract
    if tarfile.is_tarfile(dst_path):
        with tarfile.open(dst_path) as tar:
            dirs = {f.name for f in tar.getmembers() if f.isdir()}
            extract_dir = op.join(dst_base, _guess_base_dir(dirs))
            if not op.exists(extract_dir):
                logging.info("extracting %s", dst_file)
                tar.extractall(dst_base)

    elif zipfile.is_zipfile(dst_path):
        with zipfile.ZipFile(dst_path) as zip_:
            if uncompress_dir != '':
                extract_dir = uncompress_dir
                if not op.exists(extract_dir):
                    logging.info("extracting %s", dst_file)
                    zip_.extractall(extract_dir)
            else:
                dirs = {op.dirname(f) for f in zip_.namelist()}
                extract_dir = op.join(dst_base, _guess_base_dir(dirs))
                if not op.exists(extract_dir):
                    logging.info("extracting %s", dst_file)
                    zip_.extractall(dst_base)

    else:
        extract_dir = op.join(dst_base, name)
        target = op.join(dst_base, name)
        needCreateAlink = False

    if needCreateAlink:
        # Remove previous link if needed
        target = op.join(dst_base, name)
        rel_extract_dir = op.basename(extract_dir)
        if op.islink(target) and os.readlink(target) != rel_extract_dir:
            logging.info("unlink %s target", target)
            os.unlink(target)
        elif op.exists(target) and not op.islink(target):
            logging.info("remove previous %s copy", target)
            _rmtree(target)

        # Link (or copy)
        if not op.exists(target):
            logging.info("symlink %s -> %s", target, rel_extract_dir)
            try:
                os.symlink(rel_extract_dir, target)
            except OSError:
                # This typically happens on Windows when Developer Mode is not
                # available/enabled
                logging.info("unable to symlink, fallback on copy (%s -> %s)", extract_dir, target)
                shutil.copytree(extract_dir, target)

    return name, target


def _fetch_externals(args):
    dependencies = _get_external_deps(args)
    with Pool() as p:
        return dict(p.map(_download_extract, dependencies.items()))


def _block(name, prerequisites=None):
    def real_decorator(block_func):
        block_func.name = name
        block_func.prerequisites = prerequisites if prerequisites else []
        return block_func

    return real_decorator


def _get_cmake_setup_options(
    cfg,
    build_type="$(CMAKE_BUILD_TYPE)",
    generator="$(CMAKE_GENERATOR)",
    prefix="$(PREFIX)",
    external_dir="$(EXTERNAL_DIR)",
):
    opts = f'-DCMAKE_BUILD_TYPE={build_type} -G"{generator}" -DCMAKE_INSTALL_PREFIX="{prefix}" -DEXTERNAL_DIR="{external_dir}"'
    if _SYSTEM == "Windows":
        opts += f" -DCMAKE_INSTALL_INCLUDEDIR=Include -DCMAKE_INSTALL_LIBDIR=Lib -DCMAKE_INSTALL_BINDIR=Scripts"
        # Always use MultiThreadedDLL (/MD), not MultiThreadedDebugDLL (/MDd)
        # Some external libraries are only available in Release mode, not in Debug mode
        # MSVC toolchain doesn't allow mixing libraries built in /MD mode with libraries built in /MDd mode
        opts += f" -DCMAKE_MSVC_RUNTIME_LIBRARY:STRING=MultiThreadedDLL"
        # Set Windows SDK Version
        opts += f" -DCMAKE_SYSTEM_VERSION=$(CMAKE_SYSTEM_VERSION)"
        # Set VCPKG Directory
        opts += f" -DVCPKG_DIR=$(VCPKG_DIR)"
    return opts


def _get_cmake_compile_options(cfg, build_type="$(CMAKE_BUILD_TYPE)", num_threads=cpu_count() + 1):
    opts = f"--config {build_type} -j{num_threads}"
    if cfg.args.verbose:
        opts += "-v"
    return opts


def _get_cmake_install_options(cfg, build_type="$(CMAKE_BUILD_TYPE)"):
    return f"--config {build_type}"


def _meson_compile_install_cmd(component, external=False):
    builddir = op.join("external", component, "builddir") if external else op.join("$(BUILDDIR)", component)
    return ["$(MESON) " + _cmd_join(action, "-C", builddir) for action in ("compile", "install")]


@_block("pkgconf-setup")
def _pkgconf_setup(cfg):
    builddir = op.join("external", "pkgconf", "builddir")
    return ["$(MESON_SETUP) " + _cmd_join("-Dtests=false", cfg.externals["pkgconf"], builddir)]


@_block("pkgconf-install", [_pkgconf_setup])
def _pkgconf_install(cfg):
    ret = _meson_compile_install_cmd("pkgconf", external=True)
    pkgconf_exe = op.join(cfg.bin_path, "pkgconf.exe")
    pkgconfig_exe = op.join(cfg.bin_path, "pkg-config.exe")
    return ret + [f"copy {pkgconf_exe} {pkgconfig_exe}"]


@_block("shaderc-install")
def _shaderc_install(cfg):
    shaderc_cmake_setup_options = _get_cmake_setup_options(cfg, build_type="Release", generator="Ninja")
    shaderc_cmake_compile_options = _get_cmake_compile_options(cfg, build_type="Release")
    shaderc_cmake_install_options = _get_cmake_install_options(cfg, build_type="Release")
    if _SYSTEM == "Darwin":
        shaderc_lib_filename = "libshaderc_shared.1.dylib"
    cmd = []
    if _SYSTEM in ["Darwin", "Windows"]:
        cmd += ["cd external/shaderc && python ./utils/git-sync-deps"]
        cmd += [
            f"$(CMAKE) -S external/shaderc -B $(BUILDDIR)/shaderc {shaderc_cmake_setup_options} -DSHADERC_SKIP_TESTS=ON -DLLVM_USE_CRT_DEBUG=MD -DLLVM_USE_CRT_RELEASE=MD",
            f"$(CMAKE) --build $(BUILDDIR)/shaderc {shaderc_cmake_compile_options}",
            f"$(CMAKE) --install $(BUILDDIR)/shaderc {shaderc_cmake_install_options}",
        ]
        if _SYSTEM == "Darwin":
            cmd += [f"install_name_tool -id @rpath/{shaderc_lib_filename} $(PREFIX)/lib/{shaderc_lib_filename}"]
    return cmd


@_block("d3dx12_ShaderCompiler-install")
def _d3dx12_ShaderCompiler_install(cfg):
    if _SYSTEM == "Windows":
        d3d12_list_bin = op.join(_ROOTDIR, "external", "d3dx12_ShaderCompiler" , "bin", "x64", "*.*")
        return [f"(xcopy /Y \"{d3d12_list_bin}\" \"{cfg.bin_path}\")"]
    return []


@_block("d3dx12_AgilitySDK-install")
def _d3dx12_AgilitySDK_install(cfg):
    if _SYSTEM == "Windows":
        d3d12_list_bin = op.join(_ROOTDIR, "external", "d3dx12_AgilitySDK" , "build", "native", "bin", "x64", "*.*")
        d3d12_output = op.join(cfg.bin_path, "D3D12")  # Need to ouput inside D3D12 related to "d3d12_declare_agility_SDK.c"
        return [f"(xcopy /Y /I \"{d3d12_list_bin}\" \"{d3d12_output}\")"]
    return []


@_block("sxplayer-setup")
def _sxplayer_setup(cfg):
    builddir = op.join("external", "sxplayer", "builddir")
    return ["$(MESON_SETUP) -Drpath=true " + _cmd_join(cfg.externals["sxplayer"], builddir)]


@_block("sxplayer-install", [_sxplayer_setup])
def _sxplayer_install(cfg):
    return _meson_compile_install_cmd("sxplayer", external=True)


@_block("renderdoc-install")
def _renderdoc_install(cfg):
    renderdoc_dll = op.join(cfg.externals[_RENDERDOC_ID], "renderdoc.dll")
    return [f"copy {renderdoc_dll} {cfg.bin_path}"]


@_block("nodegl-setup", [_sxplayer_install, _shaderc_install, _d3dx12_ShaderCompiler_install, _d3dx12_AgilitySDK_install])
def _nodegl_setup(cfg):
    nodegl_opts = []
    if cfg.args.debug_opts:
        debug_opts = ",".join(cfg.args.debug_opts)
        nodegl_opts += [f"-Ddebug_opts={debug_opts}"]

    if "gpu_capture" in cfg.args.debug_opts:
        renderdoc_dir = cfg.externals[_RENDERDOC_ID]
        nodegl_opts += [f"-Drenderdoc_dir={renderdoc_dir}"]

    extra_library_dirs = []
    extra_include_dirs = []
    if _SYSTEM == "Windows":
        vcpkg_prefix = op.join(cfg.args.vcpkg_dir, "installed", "x64-windows")
        extra_library_dirs += [
            op.join(cfg.prefix, "Lib"),
            op.join(vcpkg_prefix, "lib"),
            op.join(_ROOTDIR, "external", "d3dx12_ShaderCompiler", "lib", "x64"),
        ]
        extra_include_dirs += [
            op.join(cfg.prefix, "Include"),
            op.join(vcpkg_prefix, "include"),
            op.join(_ROOTDIR, "external", "d3dx12_ShaderCompiler", "inc"),
            op.join(_ROOTDIR, "external", "d3dx12_AgilitySDK", "build", "native", "include"),
            op.join(_ROOTDIR, "external", "d3dx12_AgilitySDK", "build", "native", "include", "d3dx12"),
            op.join(_ROOTDIR, "external", "json"),
            op.join("src", "pch", "windows"),
        ]
    elif _SYSTEM == "Darwin":
        prefix = _get_brew_prefix()
        if prefix:
            extra_library_dirs += [op.join(prefix, "lib")]
            extra_include_dirs += [op.join(prefix, "include")]

    if extra_library_dirs:
        opts = ",".join(extra_library_dirs)
        nodegl_opts += [f"-Dextra_library_dirs={opts}"]

    if extra_include_dirs:
        opts = ",".join(extra_include_dirs)
        nodegl_opts += [f"-Dextra_include_dirs={opts}"]

    return ["$(MESON_SETUP) -Drpath=true " + _cmd_join(*nodegl_opts, "libnodegl", op.join("$(BUILDDIR)", "libnodegl"))]


@_block("nodegl-install", [_nodegl_setup])
def _nodegl_install(cfg):
    return _meson_compile_install_cmd("libnodegl")


@_block("pynodegl-deps-install", [_nodegl_install])
def _pynodegl_deps_install(cfg):
    return ["$(PIP) " + _cmd_join("install", "-r", op.join(".", "pynodegl", "requirements.txt"))]


@_block("pynodegl-install", [_pynodegl_deps_install])
def _pynodegl_install(cfg):
    ret = ["$(PIP) " + _cmd_join("-v", "install", "-e", op.join(".", "pynodegl"))]
    if _SYSTEM == "Windows":
        dlls = op.join(cfg.prefix, "Scripts", "*.dll")
        ret += [f"(xcopy /Y {dlls} pynodegl\\.)"]
    else:
        rpath = op.join(cfg.prefix, "lib")
        ldflags = f"-Wl,-rpath,{rpath}"
        ret[0] = f"LDFLAGS={ldflags} {ret[0]}"
    return ret


@_block("pynodegl-utils-deps-install", [_pynodegl_install])
def _pynodegl_utils_deps_install(cfg):
    #
    # Requirements not installed on MinGW because:
    # - PySide6 can't be pulled (required to be installed by the user outside the
    #   Python virtual env)
    # - Pillow fails to find zlib (required to be installed by the user outside the
    #   Python virtual env)
    #
    if _SYSTEM == "MinGW":
        return ["@"]  # noop
    return ["$(PIP) " + _cmd_join("install", "-r", op.join(".", "pynodegl-utils", "requirements.txt"))]


@_block("pynodegl-utils-install", [_pynodegl_utils_deps_install])
def _pynodegl_utils_install(cfg):
    return ["$(PIP) " + _cmd_join("-v", "install", "-e", op.join(".", "pynodegl-utils"))]


@_block("ngl-tools-setup", [_nodegl_install])
def _ngl_tools_setup(cfg):
    return ["$(MESON_SETUP) -Drpath=true " + _cmd_join("ngl-tools", op.join("$(BUILDDIR)", "ngl-tools"))]


@_block("ngl-tools-install", [_ngl_tools_setup])
def _ngl_tools_install(cfg):
    return _meson_compile_install_cmd("ngl-tools")


def _nodegl_run_target_cmd(cfg, target):
    builddir = op.join("$(BUILDDIR)", "libnodegl")
    return ["$(MESON) " + _cmd_join("compile", "-C", builddir, target)]


@_block("nodegl-updatedoc", [_nodegl_install])
def _nodegl_updatedoc(cfg):
    return _nodegl_run_target_cmd(cfg, "updatedoc")


@_block("nodegl-updatespecs", [_nodegl_install])
def _nodegl_updatespecs(cfg):
    return _nodegl_run_target_cmd(cfg, "updatespecs")


@_block("nodegl-updateglwrappers", [_nodegl_install])
def _nodegl_updateglwrappers(cfg):
    return _nodegl_run_target_cmd(cfg, "updateglwrappers")


@_block("all", [_ngl_tools_install, _pynodegl_utils_install])
def _all(cfg):
    echo = ["", "Build completed.", "", "You can now enter the venv with:"]
    if _SYSTEM == "Windows":
        echo.append(op.join(cfg.bin_path, "Activate.ps1"))
        return [f"@echo.{e}" for e in echo]
    else:
        echo.append(" " * 4 + ". " + op.join(cfg.bin_path, "activate"))
        return [f'@echo "    {e}"' for e in echo]


@_block("tests-setup", [_ngl_tools_install, _pynodegl_utils_install])
def _tests_setup(cfg):
    return ["$(MESON_SETUP_TESTS) " + _cmd_join("tests", op.join("builddir", "tests"))]


@_block("nodegl-tests", [_nodegl_install])
def _nodegl_tests(cfg):
    return ["$(MESON) " + _cmd_join("test", "-C", op.join("$(BUILDDIR)", "libnodegl"))]


def _rm(f):
    return f"(if exist {f} del /q {f})" if _SYSTEM == "Windows" else f"$(RM) {f}"


def _rd(d):
    return f"(if exist {d} rd /s /q {d})" if _SYSTEM == "Windows" else f"$(RM) -r {d}"


@_block("clean-py")
def _clean_py(cfg):
    return [
        _rm(op.join("pynodegl", "nodes_def.pyx")),
        _rm(op.join("pynodegl", "_pynodegl.c")),
        _rm(op.join("pynodegl", "_pynodegl.*.so")),
        _rm(op.join("pynodegl", "pynodegl.*.pyd")),
        _rm(op.join("pynodegl", "pynodegl", "__init__.py")),
        _rd(op.join("pynodegl", "build")),
        _rd(op.join("pynodegl", "pynodegl.egg-info")),
        _rd(op.join("pynodegl", ".eggs")),
        _rd(op.join("pynodegl-utils", "pynodegl_utils.egg-info")),
        _rd(op.join("pynodegl-utils", ".eggs")),
    ]


@_block("clean", [_clean_py])
def _clean(cfg):
    return [
        _rd(op.join("$(BUILDDIR)", "libnodegl")),
        _rd(op.join("$(BUILDDIR)", "ngl-tools")),
        _rd(op.join("$(BUILDDIR)", "tests")),
        _rd(op.join("$(BUILDDIR)", "shaderc")),
        _rd(op.join("external", "pkgconf", "builddir")),
        _rd(op.join("external", "sxplayer", "builddir")),
        _rd(op.join("external", "shaderc", "builddir")),
    ]


def _coverage(cfg, output):
    # We don't use `meson coverage` here because of
    # https://github.com/mesonbuild/meson/issues/7895
    return [_cmd_join("ninja", "-C", op.join("$(BUILDDIR)", "libnodegl"), f"coverage-{output}")]


@_block("coverage-html")
def _coverage_html(cfg):
    return _coverage(cfg, "html")


@_block("coverage-xml")
def _coverage_xml(cfg):
    return _coverage(cfg, "xml")


@_block("tests", [_nodegl_tests, _tests_setup])
def _tests(cfg):
    return ["$(MESON) " + _cmd_join("test", "--timeout-multiplier", "2", "-C", op.join("$(BUILDDIR)", "tests"))]


def _quote(s):
    if not s or " " in s:
        return f'"{s}"'
    assert "'" not in s
    assert '"' not in s
    return s


def _cmd_join(*cmds):
    if _SYSTEM == "Windows":
        return " ".join(_quote(cmd) for cmd in cmds)
    return shlex.join(cmds)


def _get_make_vars(cfg):
    debug = cfg.args.coverage or cfg.args.buildtype == "debug"

    # We don't want Python to fallback on one found in the PATH so we explicit
    # it to the one in the venv.
    python = op.join(cfg.bin_path, "python")

    #
    # MAKEFLAGS= is a workaround (not working on Windows due to incompatible Make
    # syntax) for the issue described here:
    # https://github.com/ninja-build/ninja/issues/1139#issuecomment-724061270
    #
    # Note: this will invoke the meson in the venv, unless we're on MinGW where
    # it will fallback on the system one. This is due to the extended PATH
    # mechanism.
    #
    meson = "MAKEFLAGS= meson" if _SYSTEM != "Windows" else "meson"
    cmake = "cmake"
    builddir = os.getenv("BUILDDIR", "builddir")

    meson_setup = [
        "setup",
        "--prefix",
        cfg.prefix,
        "--pkg-config-path",
        cfg.pkg_config_path,
        "--buildtype",
        "debug" if debug else "release",
        "--debug",
    ]
    if cfg.args.coverage:
        meson_setup += ["-Db_coverage=true"]
    if _SYSTEM != "MinGW" and "debug" not in cfg.args.buildtype:
        meson_setup += ["-Db_lto=true"]

    cmake_builddtype = "Debug" if debug else "Release"
    cmake_setup = [
        f"-DCMAKE_INSTALL_PREFIX={cfg.prefix}",
        f"-DCMAKE_BUILD_TYPE={cmake_builddtype}",
        "-GNinja",
    ]
    if _SYSTEM == "Windows":
        meson_setup += ["--bindir=Scripts", "--libdir=Lib", "--includedir=Include"]
    elif op.isfile("/etc/debian_version"):
        # Workaround Debian/Ubuntu bug; see https://github.com/mesonbuild/meson/issues/5925
        meson_setup += ["--libdir=lib"]

    if _SYSTEM == "Windows":
        # Use Multithreaded DLL runtime library for debug and release configurations
        # Third party libs are compiled in release mode
        # Visual Studio toolchain requires all libraries to use the same runtime library
        meson_setup += ["-Db_vscrt=md"]
    ret = dict(
        BUILDDIR=builddir,
        PREFIX=cfg.prefix,  # .replace('\\','/'),
        PIP=_cmd_join(python, "-m", "pip"),
        MESON=meson,
        CMAKE=cmake,
        CMAKE_GENERATOR=cfg.cmake_generator,
        CMAKE_BUILD_TYPE=cfg.cmake_build_type,
    )

    ret["MESON_SETUP"] = "$(MESON) " + _cmd_join(*meson_setup, f"--backend={cfg.args.build_backend}")
    # Our tests/meson.build logic is not well supported with the VS backend so
    # we need to fallback on Ninja
    ret["MESON_SETUP_TESTS"] = "$(MESON) " + _cmd_join(*meson_setup, "--backend=ninja")

    ret["CMAKE_SETUP"] = "cmake " + _cmd_join(*cmake_setup)

    if _SYSTEM == "Windows":
        ret["CMAKE_SYSTEM_VERSION"] = cfg.cmake_system_version
        ret["VCPKG_DIR"] = cfg.args.vcpkg_dir
    return ret


def _get_makefile_rec(cfg, blocks, declared):
    ret = ""
    for block in blocks:
        if block.name in declared:
            continue
        declared |= {block.name}
        req_names = " ".join(r.name for r in block.prerequisites)
        req = f" {req_names}" if req_names else ""
        commands = "\n".join("\t" + cmd for cmd in block(cfg))
        ret += f"{block.name}:{req}\n{commands}\n"
        ret += _get_makefile_rec(cfg, block.prerequisites, declared)
    return ret


def _get_makefile(cfg, blocks):
    env = cfg.get_env()
    env_vars = {k: _quote(v) for k, v in env.items()}
    if _SYSTEM == "Windows":
        #
        # Environment variables are altered if and only if they already exists
        # in the environment. While this is (usually) true for PATH, it isn't
        # for the others we're trying to declare. This "if [set...]" trick is
        # to circumvent this issue.
        #
        # See https://stackoverflow.com/questions/38381422/how-do-i-set-an-environment-variables-with-nmake
        #
        vars_export = "\n".join(f"{k} = {v}" for k, v in env_vars.items()) + "\n"
        vars_export_cond = " || ".join(f"[set {k}=$({k})]" for k in env_vars.keys())
        vars_export += f"!if {vars_export_cond}\n!endif\n"
    else:
        # We must immediate assign with ":=" since sometimes values contain
        # reference to themselves (typically PATH)
        vars_export = "\n".join(f"export {k} := {v}" for k, v in env_vars.items()) + "\n"

    make_vars = _get_make_vars(cfg)
    ret = "\n".join(f"{k} = {v}" for k, v in make_vars.items()) + "\n" + vars_export

    declared = set()
    ret += _get_makefile_rec(cfg, blocks, declared)
    ret += ".PHONY: " + " ".join(declared) + "\n"
    return ret


class _EnvBuilder(venv.EnvBuilder):
    def __init__(self):
        super().__init__(system_site_packages=_SYSTEM == "MinGW", with_pip=True, prompt="nodegl")

    def post_setup(self, context):
        if _SYSTEM == "MinGW":
            pip_install = [context.env_exe, "-m", "pip", "install"]
            pip_install += ["certifi"]
            logging.info("install certificate dependencies: %s", _cmd_join(*pip_install))
            run(pip_install, check=True)
            os.environ["REQUESTS_CA_BUNDLE"] = certifi.where()
            os.environ["SSL_CERT_FILE"] = certifi.where()
            return
        pip_install = [context.env_exe, "-m", "pip", "install"]
        pip_install += ["meson", "ninja"]
        logging.info("install build dependencies: %s", _cmd_join(*pip_install))
        run(pip_install, check=True)


def _build_venv(args):
    if op.exists(args.venv_path):
        logging.warning("Python virtual env already exists at %s", args.venv_path)
        return
    logging.info("creating Python virtualenv: %s", args.venv_path)
    _EnvBuilder().create(args.venv_path)


class _Config:
    def __init__(self, args):
        self.args = args
        self.prefix = op.abspath(args.venv_path)

        # On MinGW we need the path translated from C:\ to /c/, because when
        # part of the PATH, the ':' separator will break
        if _SYSTEM == "MinGW":
            self.prefix = run(["cygpath", "-u", self.prefix], capture_output=True, text=True).stdout.strip()

        self.bin_name = "Scripts" if _SYSTEM == "Windows" else "bin"
        self.bin_path = op.join(self.prefix, self.bin_name)
        if _SYSTEM == "Windows":
            vcpkg_pc_path = op.join(args.vcpkg_dir, "installed", "x64-windows", "lib", "pkgconfig")
            self.pkg_config_path = os.pathsep.join((vcpkg_pc_path, op.join(self.prefix, "Lib", "pkgconfig")))
        else:
            self.pkg_config_path = op.join(self.prefix, "lib", "pkgconfig")
        self.externals = _fetch_externals(args)


        self.cmake_generator = os.getenv(
            "CMAKE_GENERATOR",
            {"Windows": "Visual Studio 17 2022", "Linux": "Ninja", "Darwin": "Xcode", "MinGW": "Ninja"}[_SYSTEM],
        )
        if args.buildtype == "debug":
            self.cmake_build_type = "Debug"
        else:
            self.cmake_build_type = "Release"
        if _SYSTEM == "Windows":
            # Set Windows SDK Version
            self.cmake_system_version = os.getenv("CMAKE_SYSTEM_VERSION", "10.0.22000.0")

            _sxplayer_setup.prerequisites.append(_pkgconf_install)
            if "gpu_capture" in args.debug_opts:
                _nodegl_setup.prerequisites.append(_renderdoc_install)

            vcpkg_bin = op.join(args.vcpkg_dir, "installed", "x64-windows", "bin")
            for f in glob.glob(op.join(vcpkg_bin, "*.dll")):
                logging.info("copy %s to venv/Scripts", f)
                shutil.copy2(f, op.join("venv", "Scripts"))

    def get_env(self):
        sep = ":" if _SYSTEM == "MinGW" else os.pathsep
        env = {}
        env["PATH"] = sep.join((self.bin_path, "$(PATH)"))
        env["EXTERNAL_DIR"] = op.join(_ROOTDIR, "external")
        env["PKG_CONFIG_PATH"] = self.pkg_config_path
        if _SYSTEM == "Windows":
            env["PKG_CONFIG_ALLOW_SYSTEM_LIBS"] = "1"
            env["PKG_CONFIG_ALLOW_SYSTEM_CFLAGS"] = "1"
        elif _SYSTEM == "MinGW":
            # See https://setuptools.pypa.io/en/latest/deprecated/distutils-legacy.html
            env["SETUPTOOLS_USE_DISTUTILS"] = "stdlib"
        return env


def _run():
    default_build_backend = "ninja" if _SYSTEM != "Windows" else "vs"
    parser = argparse.ArgumentParser(
        description="Create and manage a standalone node.gl virtual environement",
    )
    parser.add_argument("-p", "--venv-path", default=op.join(_ROOTDIR, "venv"), help="Virtual environment directory")
    parser.add_argument("--buildtype", choices=("release", "debug"), default="release", help="Build type")
    parser.add_argument("--coverage", action="store_true", help="Code coverage")
    parser.add_argument(
        "-d",
        "--debug-opts",
        nargs="+",
        default=[],
        choices=("gl", "vk", "d3d12", "mem", "scene", "gpu_capture", "d3d12_debug_all", "d3d12_debug_trace", "d3d12_validation"),
        help="Debug options",
    )
    parser.add_argument(
        "--build-backend", choices=("ninja", "vs", "xcode"), default=default_build_backend, help="Build backend to use"
    )
    parser.add_argument("-v", "--verbose", action="store_true", help="Enable verbose output")

    if _SYSTEM == "Windows":
        parser.add_argument("--vcpkg-dir", default=r"C:\vcpkg", help="Vcpkg directory")

    args = parser.parse_args()

    logging.basicConfig(level="INFO")

    _build_venv(args)

    dst_makefile = op.join(_ROOTDIR, "Makefile")
    logging.info("writing %s", dst_makefile)
    cfg = _Config(args)
    blocks = [
        _all,
        _tests,
        _clean,
        _nodegl_updatedoc,
        _nodegl_updatespecs,
        _nodegl_updateglwrappers,
    ]
    if _SYSTEM == "Darwin" or _SYSTEM == "Windows" :
        blocks += [_shaderc_install]
    if _SYSTEM == "Windows" :
        blocks += [_d3dx12_ShaderCompiler_install, _d3dx12_AgilitySDK_install]
    if args.coverage:
        blocks += [_coverage_html, _coverage_xml]
    makefile = _get_makefile(cfg, blocks)
    with open(dst_makefile, "w") as f:
        f.write(makefile)


if __name__ == "__main__":
    _run()
