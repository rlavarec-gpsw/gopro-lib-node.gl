#!/usr/bin/env python3
#
# Copyright 2021 GoPro Inc.
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
import logging
import os
import os.path as op
import pathlib
import platform
import shlex
import shutil
import sys
import sysconfig
import tarfile
import urllib.request
import venv
import zipfile
from multiprocessing import Pool
from subprocess import call, check_output


_ROOTDIR = op.abspath(op.dirname(__file__))
_SYSTEM = 'MinGW' if sysconfig.get_platform() == 'mingw' else platform.system()

_RENDERDOC_FILE = 'RenderDoc_@VERSION@_64.zip' if _SYSTEM == 'Windows' else 'renderdoc_@VERSION@.tar.gz'
_EXTERNAL_DEPS = dict(
    sxplayer=dict(
        version='9.8.1',
        url='https://github.com/Stupeflix/sxplayer/archive/v@VERSION@.tar.gz',
        dst_file='sxplayer-@VERSION@.tar.gz',
    ),
    pkgconf=dict(
        version='1.7.3',
        url='https://distfiles.dereferenced.org/pkgconf/pkgconf-@VERSION@.tar.xz',
    ),
    renderdoc=dict(
        version='1.12',
        url=f'https://renderdoc.org/stable/@VERSION@/{_RENDERDOC_FILE}',
    ),
)


def _guess_base_dir(dirs):
    smallest_dir = sorted(dirs, key=lambda x: len(x))[0]
    return pathlib.Path(smallest_dir).parts[0]


def _download_extract(dep_item):
    logging.basicConfig(level='INFO')  # Needed for every process on Windows

    name, dep = dep_item

    version = dep['version']
    url = dep['url'].replace('@VERSION@', version)
    dst_file = dep.get('dst_file', op.basename(url)).replace('@VERSION@', version)
    dst_base = op.join(_ROOTDIR, 'external')
    dst_path = op.join(dst_base, dst_file)
    os.makedirs(dst_base, exist_ok=True)

    # Download
    if not op.exists(dst_path):
        logging.info('downloading %s to %s', url, dst_file)
        urllib.request.urlretrieve(url, dst_path)

    # Extract
    if tarfile.is_tarfile(dst_path):
        with tarfile.open(dst_path) as tar:
            dirs = {f.name for f in tar.getmembers() if f.isdir()}
            extract_dir = op.join(dst_base, _guess_base_dir(dirs))
            if not op.exists(extract_dir):
                logging.info('extracting %s', dst_file)
                tar.extractall(dst_base)

    elif zipfile.is_zipfile(dst_path):
        with zipfile.ZipFile(dst_path) as zip_:
            dirs = {op.dirname(f) for f in zip_.namelist()}
            extract_dir = op.join(dst_base, _guess_base_dir(dirs))
            if not op.exists(extract_dir):
                logging.info('extracting %s', dst_file)
                zip_.extractall(dst_base)
    else:
        assert False

    # Link
    target = op.join(dst_base, name)
    if _SYSTEM in ('Windows', 'MinGW'):
        logging.info(f'copy {target} -> {extract_dir}')
        shutil.copytree(extract_dir, target, dirs_exist_ok=True)
    else:
        extract_dir = op.basename(extract_dir)
        if op.exists(target) and os.readlink(target) != extract_dir:
            os.unlink(target)
        if not op.exists(target):
            logging.info(f'symlink {target} -> {extract_dir}')
            os.symlink(extract_dir, target)
    return name, target


def _fetch_externals(args):
    dependencies = _EXTERNAL_DEPS.copy()
    if _SYSTEM not in {'Windows', 'MinGW'}:
        del dependencies['pkgconf']
    if 'gpu_capture' not in args.debug:
        del dependencies['renderdoc']

    with Pool() as p:
        return dict(p.map(_download_extract, dependencies.items()))


def _block(name, prerequisites=None):
    def real_decorator(block_func):
        block_func.name = name
        block_func.prerequisites = prerequisites if prerequisites else []
        return block_func
    return real_decorator


def _meson_compile_install_cmd(component):
    builddir = op.join('builddir', component)
    return ['$(MESON) ' + _cmd_join(action, '-C', builddir) for action in ('compile', 'install')]


@_block('pkgconf-setup')
def _pkgconf_setup(cfg):
    return ['$(MESON_SETUP) ' + _cmd_join('-Dtests=false', cfg.externals['pkgconf'], op.join('builddir', 'pkgconf'))]


@_block('pkgconf-install', [_pkgconf_setup])
def _pkgconf_install(cfg):
    ret = _meson_compile_install_cmd('pkgconf')
    pkgconf_exe = op.join(cfg.bin_path, 'pkgconf.exe')
    pkgconfig_exe = op.join(cfg.bin_path, 'pkg-config.exe')
    return ret + [f'copy {pkgconf_exe} {pkgconfig_exe}']


@_block('sxplayer-setup')
def _sxplayer_setup(cfg):
    return ['$(MESON_SETUP) ' + _cmd_join(cfg.externals['sxplayer'], op.join('builddir', 'sxplayer'))]


@_block('sxplayer-install', [_sxplayer_setup])
def _sxplayer_install(cfg):
    return _meson_compile_install_cmd('sxplayer')


@_block('renderdoc-install')
def _renderdoc_install(cfg):
    renderdoc_dll = op.join(cfg.externals['renderdoc'], 'renderdoc.dll')
    return [f'copy {renderdoc_dll} {cfg.bin_path}']


@_block('nodegl-setup', [_sxplayer_install])
def _nodegl_setup(cfg):
    nodegl_debug_opts = []
    if cfg.args.debug:
        debug_opts = ','.join(cfg.args.debug)
        nodegl_debug_opts += [f'-Ddebug_opts={debug_opts}']

    if 'gpu_capture' in cfg.args.debug:
        renderdoc_dir = cfg.externals['renderdoc']
        nodegl_debug_opts += [f'-Drenderdoc_dir={renderdoc_dir}']

    return ['$(MESON_SETUP) ' + _cmd_join(*nodegl_debug_opts, 'libnodegl', op.join('builddir', 'libnodegl'))]


@_block('nodegl-install', [_nodegl_setup])
def _nodegl_install(cfg):
    return _meson_compile_install_cmd('libnodegl')


@_block('pynodegl-deps-install', [_nodegl_install])
def _pynodegl_deps_install(cfg):
    return ['$(PIP) ' + _cmd_join('install', '-r', op.join('.', 'pynodegl', 'requirements.txt'))]


@_block('pynodegl-install', [_pynodegl_deps_install])
def _pynodegl_install(cfg):
    ret = ['$(PIP) ' + _cmd_join('-v', 'install', '-e', op.join('.', 'pynodegl'))]
    if _SYSTEM == 'Windows':
        dlls = op.join(cfg.prefix, 'Scripts', '*.dll')
        ret += [f'xcopy /Y {dlls} pynodegl\\.']
    else:
        rpath = op.join(cfg.prefix, 'lib')
        ldflags = f'-Wl,-rpath,{rpath}'
        ret[0] = f'LDFLAGS={ldflags} {ret[0]}'
    return ret


@_block('pynodegl-utils-deps-install', [_pynodegl_install])
def _pynodegl_utils_deps_install(cfg):
    #
    # Requirements not installed on MinGW because:
    # - PySide2 can't be pulled (required to be installed by the user outside the
    #   Python virtual env)
    # - Pillow fails to find zlib (required to be installed by the user outside the
    #   Python virtual env)
    #
    if _SYSTEM == 'MinGW':
        return ['@']  # noop
    return ['$(PIP) ' + _cmd_join('install', '-r', op.join('.', 'pynodegl-utils', 'requirements.txt'))]


@_block('pynodegl-utils-install', [_pynodegl_utils_deps_install])
def _pynodegl_utils_install(cfg):
    return ['$(PIP) ' + _cmd_join('-v', 'install', '-e', op.join('.', 'pynodegl-utils'))]


@_block('ngl-tools-setup', [_nodegl_install])
def _ngl_tools_setup(cfg):
    return ['$(MESON_SETUP) ' + _cmd_join('ngl-tools', op.join('builddir', 'ngl-tools'))]


@_block('ngl-tools-install', [_ngl_tools_setup])
def _ngl_tools_install(cfg):
    ret = _meson_compile_install_cmd('ngl-tools')
    if _SYSTEM == 'Windows':
        bindir = cfg.bin_path
        ngl_tools_dlls = op.join('builddir', 'ngl-tools', '*.dll')
        ret += [f'xcopy /Y {ngl_tools_dlls} {bindir}']
    return ret


def _nodegl_run_target_cmd(cfg, target):
    builddir = op.join('builddir', 'libnodegl')
    return ['$(MESON) ' + _cmd_join('compile', '-C', builddir, target)]


@_block('nodegl-updatedoc', [_nodegl_install])
def _nodegl_updatedoc(cfg):
    return _nodegl_run_target_cmd(cfg, 'updatedoc')


@_block('nodegl-updatespecs', [_nodegl_install])
def _nodegl_updatespecs(cfg):
    return _nodegl_run_target_cmd(cfg, 'updatespecs')


@_block('nodegl-updateglwrappers', [_nodegl_install])
def _nodegl_updateglwrappers(cfg):
    return _nodegl_run_target_cmd(cfg, 'updateglwrappers')


@_block('all', [_ngl_tools_install, _pynodegl_utils_install])
def _all(cfg):
    if _SYSTEM == 'Windows':
        activate = op.join(cfg.bin_path, 'Activate.ps1')
    else:
        activate = '. ' + op.join(cfg.bin_path, 'activate')
    return [
        '@echo',
        '@echo "    Build completed."',
        '@echo',
        '@echo "    You can now enter the venv with:"',
        f'@echo "        {activate}"',
    ]


@_block('tests-setup', [_ngl_tools_install, _pynodegl_utils_install])
def _tests_setup(cfg):
    return ['$(MESON_SETUP_TESTS) ' + _cmd_join('tests', op.join('builddir', 'tests'))]


@_block('nodegl-tests', [_nodegl_install])
def _nodegl_tests(cfg):
    return [_cmd_join('meson', 'test', '-C', op.join('builddir', 'libnodegl'))]


@_block('clean-py')
def _clean_py(cfg):
    return [
        '$(RM) ' + op.join('pynodegl', 'nodes_def.pyx'),
        '$(RM) ' + op.join('pynodegl', 'pynodegl.c'),
        '$(RM) ' + op.join('pynodegl', 'pynodegl.*.so'),
        '$(RM) -r ' + op.join('pynodegl', 'build'),
        '$(RM) -r ' + op.join('pynodegl', 'pynodegl.egg-info'),
        '$(RM) -r ' + op.join('pynodegl', '.eggs'),
        '$(RM) -r ' + op.join('pynodegl-utils', 'pynodegl_utils.egg-info'),
        '$(RM) -r ' + op.join('pynodegl-utils', '.eggs'),
    ]


@_block('clean', [_clean_py])
def _clean(cfg):
    return [
        '$(RM) -r ' + op.join('builddir', 'sxplayer'),
        '$(RM) -r ' + op.join('builddir', 'libnodegl'),
        '$(RM) -r ' + op.join('builddir', 'ngl-tools'),
        '$(RM) -r ' + op.join('builddir', 'tests'),
    ]


def _coverage(cfg, output):
    # We don't use `meson coverage` here because of
    # https://github.com/mesonbuild/meson/issues/7895
    ninja = op.join(cfg.prefix, 'ninja')
    return [_cmd_join(ninja, '-C', op.join('builddir', 'libnodegl'), f'coverage-{output}')]


@_block('coverage-html')
def _coverage_html(cfg):
    return _coverage(cfg, 'html')


@_block('coverage-xml')
def _coverage_xml(cfg):
    return _coverage(cfg, 'xml')


# XXX: meson tests suite args?
@_block('tests', [_nodegl_tests, _tests_setup])
def _tests(cfg):
    return [_cmd_join('meson', 'test', '-C', op.join('builddir', 'tests'))]


def _quote(s):
    if not s or ' ' in s:
        return f'"{s}"'
    assert "'" not in s
    assert '"' not in s
    return s


def _cmd_join(*cmds):
    if _SYSTEM == 'Windows':
        return ' '.join(_quote(cmd) for cmd in cmds)
    return shlex.join(cmds)


def _get_make_vars(cfg):
    python = op.join(cfg.bin_path, 'python')

    # MAKEFLAGS= is a workaround (not working on Windows due to incompatible Make
    # syntax) for the issue described here:
    # https://github.com/ninja-build/ninja/issues/1139#issuecomment-724061270
    meson = 'MAKEFLAGS= meson' if _SYSTEM != 'Windows' else 'meson'

    buildtype = 'debugoptimized' if cfg.args.coverage or cfg.args.buildtype == 'debug' else 'release'
    meson_setup = [
        'setup',
        '--prefix', cfg.prefix,
        '-Drpath=true',
        '--pkg-config-path', cfg.pkg_config_path,
        '--buildtype', buildtype,
    ]
    if cfg.args.coverage:
        meson_setup += ['-Db_coverage=true']
    if _SYSTEM != 'MinGW':
        meson_setup += ['-Db_lto=true']

    if _SYSTEM == 'Windows':
        meson_setup += ['--bindir=Scripts', '--libdir=Lib', '--includedir=Include']

    if _SYSTEM == 'Windows':
        # Use Multithreaded DLL runtime library for debug and release configurations
        # Third party libs are compiled in release mode
        # Visual Studio toolchain requires all libraries to use the same runtime library
        meson_setup += ['-Db_vscrt=md']
    else:
        # Workaround Debian/Ubuntu bug; see https://github.com/mesonbuild/meson/issues/5925
        try:
            distrib_id = check_output(['lsb_release', '-si']).strip().decode()
        except Exception:
            pass
        else:
            if distrib_id in ('Ubuntu', 'Debian'):
                meson_setup += ['--libdir=lib']

    ret = dict(
        PIP=_cmd_join(python, '-m', 'pip'),
        MESON=meson,
    )

    # Our tests/meson.build logic is not well supported with the VS backend so
    # we need to fallback on Ninja
    if _SYSTEM == 'Windows':
        ret['MESON_SETUP'] = '$(MESON) ' + _cmd_join(*meson_setup, '--backend=vs')
        ret['MESON_SETUP_TESTS'] = '$(MESON) ' + _cmd_join(*meson_setup, '--backend=ninja')
    else:
        ret['MESON_SETUP'] = '$(MESON) ' + _cmd_join(*meson_setup)
        ret['MESON_SETUP_TESTS'] = '$(MESON_SETUP)'

    return ret


def _get_makefile_rec(cfg, blocks, declared):
    ret = ''
    for block in blocks:
        if block.name in declared:
            continue
        declared |= {block.name}
        req_names = ' '.join(r.name for r in block.prerequisites)
        req = f' {req_names}' if req_names else ''
        commands = '\n'.join('\t' + cmd for cmd in block(cfg))
        ret += f'{block.name}:{req}\n{commands}\n'
        ret += _get_makefile_rec(cfg, block.prerequisites, declared)
    return ret


def _get_makefile(cfg, blocks):
    env = cfg.get_env()
    env_vars = {k: _quote(v) for k, v in env.items()}
    if _SYSTEM == 'Windows':
        #
        # Environment variables are altered if and only if they already exists
        # in the environment. While this is (usually) true for PATH, it isn't
        # for the others we're trying to declare. This "if [set...]" trick is
        # to circumvent this issue.
        #
        # See https://stackoverflow.com/questions/38381422/how-do-i-set-an-environment-variables-with-nmake
        #
        vars_export = '\n'.join(f'{k} = {v}' for k, v in env_vars.items()) + '\n'
        vars_export_cond = ' || '.join(f'[set {k}=$({k})]' for k in env_vars.keys())
        vars_export += f'!if {vars_export_cond}\n!endif\n'
    else:
        # We must immediate assign with ":=" since sometimes values contain
        # reference to themselves (typically PATH)
        vars_export = '\n'.join(f'export {k} := {v}' for k, v in env_vars.items()) + '\n'

    make_vars = _get_make_vars(cfg)
    ret = '\n'.join(f'{k} = {v}' for k, v in make_vars.items()) + '\n' + vars_export

    declared = set()
    ret += _get_makefile_rec(cfg, blocks, declared)
    ret += '.PHONY: ' + ' '.join(declared) + '\n'
    return ret


def _cmd(*args, _env=None):
    logging.info('CMD: ' + _cmd_join(*args))
    ret = call(args, env=_env)
    if ret:
        sys.exit(ret)


class _EnvBuilder(venv.EnvBuilder):

    def __init__(self):
        super().__init__(
            system_site_packages=_SYSTEM == 'MinGW',
            with_pip=True,
            prompt='nodegl',
        )

    def post_setup(self, context):
        pip_install = [context.env_exe, '-m', 'pip', 'install']
        if _SYSTEM != 'MinGW':
            _cmd(*pip_install, 'meson', 'ninja')


def _build_venv(args):
    if op.exists(args.venv_path):
        logging.warning('Python virtual env already exists at %s', args.venv_path)
        return
    logging.info('creating Python virtualenv: %s', args.venv_path)
    builder = _EnvBuilder()
    builder.create(args.venv_path)


class _Config:

    def __init__(self, args):
        self.args = args
        self.prefix = op.abspath(args.venv_path)

        # On MinGW we need the path translated from C:\ to /c/, because when
        # part of the PATH, the ':' separator will break
        if _SYSTEM == 'MinGW':
            self.prefix = check_output(['cygpath', '-u', self.prefix]).strip().decode()

        self.bin_name = 'Scripts' if _SYSTEM == 'Windows' else 'bin'
        self.bin_path = op.join(self.prefix, self.bin_name)
        if _SYSTEM == 'Windows':
            vcpkg_pc_path = op.join(args.vcpkg_dir, 'installed', 'x64-windows', 'lib', 'pkgconfig')
            self.pkg_config_path = os.pathsep.join((vcpkg_pc_path, op.join(self.prefix, 'Lib', 'pkgconfig')))
        else:
            self.pkg_config_path = op.join(self.prefix, 'lib', 'pkgconfig')
        self.externals = _fetch_externals(args)

        if _SYSTEM == 'Windows':
            _sxplayer_setup.prerequisites.append(_pkgconf_install)
            if 'gpu_capture' in args.debug:
                _nodegl_setup.prerequisites.append(_renderdoc_install)

            vcpkg_bin = op.join(args.vcpkg_dir, 'installed', 'x64-windows', 'bin')
            for f in glob.glob(op.join(vcpkg_bin, '*.dll')):
                logging.info('copy %s to venv/Scripts', f)
                shutil.copy2(f, op.join('venv', 'Scripts'))

    def get_env(self):
        sep = ':' if _SYSTEM == 'MinGW' else os.pathsep
        env = {}
        env['PATH'] = sep.join((self.bin_path, '$(PATH)'))
        env['PKG_CONFIG_PATH'] = self.pkg_config_path
        if _SYSTEM == 'Windows':
            env['PKG_CONFIG_ALLOW_SYSTEM_LIBS'] = '1'
            env['PKG_CONFIG_ALLOW_SYSTEM_CFLAGS'] = '1'
        return env


def _run():
    parser = argparse.ArgumentParser(
        prog='ngl-env',
        description='Create and manage a standalone node.gl virtual environement',
    )
    parser.add_argument('-p', '--venv-path', default=op.join(_ROOTDIR, 'venv'),
                        help='Virtual environment directory')
    parser.add_argument('--buildtype', choices=('release', 'debug'), default='release',
                        help='Build type')
    parser.add_argument('--coverage', action='store_true',
                        help='Code coverage')
    parser.add_argument('-d', '--debug', nargs='+', default=[],
                        choices=('gl', 'mem', 'scene', 'gpu_capture'),
                        help='Debug options')
    if _SYSTEM == 'Windows':
        parser.add_argument('--vcpkg-dir', default=r'C:\vcpkg',
                            help='Vcpkg directory')

    args = parser.parse_args()

    logging.basicConfig(level='INFO')

    _build_venv(args)

    dst_makefile = op.join(_ROOTDIR, 'Makefile')
    logging.info('writing %s', dst_makefile)
    cfg = _Config(args)
    blocks = [
        _all, _tests, _clean,
        _nodegl_updatedoc, _nodegl_updatespecs, _nodegl_updateglwrappers,
    ]
    if args.coverage:
        blocks += [_coverage_html, _coverage_xml]
    makefile = _get_makefile(cfg, blocks)
    with open(dst_makefile, 'w') as f:
        f.write(makefile)


if __name__ == '__main__':
    _run()
