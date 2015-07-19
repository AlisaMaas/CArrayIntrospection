########################################################################
#
#  configurable paths to various supporting tools
#

from os import access, environ, R_OK, X_OK
from SCons.Script import *

def pathIsExecutable(key, val, env):
    found = env.WhereIs(val)
    if found: val = found
    PathVariable.PathIsFile(key, val, env)
    if not access(val, X_OK):
        raise SCons.Errors.UserError('Path for option %s is not executable: %s' % (key, val))

def pathIsOptionalExecutable(key, val, env):
    if val:
        pathIsExecutable(key, val, env)

def pathIsSRA(key, val, env):
    problems = []
    def check(perms, diagnostic, subpath):
        node = File(subpath, val)
        if not access(node.path, perms):
            problems.append('%s: %s' % (diagnostic, node))

    # check a representative sample of what we will need from
    # llvm-sra, but don't bother checking absolutely everything
    check(X_OK, 'not executable', 'bin/sage-opt')
    check(R_OK, 'missing or unreadable', 'lib/SRA.so')
    check(R_OK, 'missing or unreadable', 'Runtime/llvmsage/__init__.py')

    if problems:
        problems = '\n\t'.join(problems)
        raise SCons.Errors.UserError('Invalid path for option %s:\n\t%s\n' % (key, problems))

variables = Variables(['.scons-options'], ARGUMENTS)
variables.Add(PathVariable('IIGLUE', 'Path to iiglue executable', '/p/polyglot/public/bin/iiglue', pathIsOptionalExecutable))

llvmConfigDefault = WhereIs('llvm-config', (
    '/p/polyglot/public/bin',
    '/usr/bin',
)) or '/usr/bin/llvm-config'
variables.Add(PathVariable('LLVM_CONFIG', 'Path to llvm-config executable', llvmConfigDefault, pathIsExecutable))

sageDefault = WhereIs('sage', (
    '/p/polyglot/public/bin',
    '/usr/bin',
)) or '/usr/bin/sage'
variables.Add(PathVariable('SAGE', 'Path to sage executable', sageDefault, pathIsExecutable))


########################################################################
#
#  common basis for all build environments
#

env = Environment(
    tools=(
        'default',              # load first, so others can override
        'bitcode',
        'clang-analyzer',
        'expect',
        'iiglue',
        'plugin',
        'textfile',
    ),
    toolpath=(
        'scons-tools',
    ),
    variables=variables,
)

Help(variables.GenerateHelpText(env))
variables.Save('.scons-options', env)


########################################################################
#
#  LLVM configuration
#

from distutils.version import StrictVersion

def llvm_version(context):
    context.Message('checking LLVM version ... ')
    succeeded, output = context.TryAction('$LLVM_CONFIG --version >$TARGET')
    if succeeded:
        result = output.rstrip('\n')
        context.env['llvm_version'] = result
        context.Result(result)
        return result
    else:
        context.Result('failed')
        context.env.Exit(1)

def llvm_bindir(context):
    context.Message('checking LLVM executables ... ')
    succeeded, output = context.TryAction('$LLVM_CONFIG --bindir >$TARGET')
    if succeeded:
        output = output.rstrip()
        context.env['LLVM_BINDIR'] = output
        context.env.PrependENVPath('PATH', output)
        context.Result(output)
        return output
    else:
        context.Result('failed')
        context.env.Exit(1)

conf = Configure(env, custom_tests={
        'LLVMVersion': llvm_version,
        'LLVMBinDir': llvm_bindir,
        })

conf.LLVMVersion()
conf.LLVMBinDir()
env = conf.Finish()


########################################################################
#
#  build environment for compiling LLVM plugins
#

penv = env.Clone(
    CPPPATH=(
        '/unsup/boost-1.55.0/common/include',
    ),
    INCPREFIX='-isystem ',
    LIBS=('LLVM-$llvm_version',),
    SHLIBPREFIX=None,
)
penv.ParseConfig('$LLVM_CONFIG --cxxflags --ldflags')
penv.AppendUnique(
    CCFLAGS=('-fexceptions', '-frtti'),
    CXXFLAGS=('-Wall', '-Wextra', '-Werror'),
    delete_existing=True
)
penv.PrependENVPath('PATH', '/s/gcc-5.1.0/bin')


########################################################################
#
#  specific plugins
#

python_plugin = penv.SharedLibrary(
    'SRA/SAGE/Python/Python',
    Glob('SRA/SAGE/Python/*.cpp'),
    CCFLAGS=(
        '$CCFLAGS',
        '-Wno-cast-qual',
    ),
    parse_flags='!python-config --libs',
)

sage_plugin = penv.SharedLibrary(
    'SRA/SAGE/SAGE',
    Glob('SRA/SAGE/*.cpp'),
    CCFLAGS=(
        '$CCFLAGS',
        '-Wno-unused-parameter',
    ),
    LIBS=python_plugin,
)

sra_plugin = penv.SharedLibrary(
    'SRA/SRA',
    Glob('SRA/*.cpp'),
    CCFLAGS=(
        '$CCFLAGS',
        '-Wno-maybe-uninitialized',
        '-Wno-unknown-warning-option',
        '-Wno-unused-parameter',
    ),
    LIBS=sage_plugin,
)

run_script = penv.Substfile(
    'run.in',
    SUBST_DICT={
        '@LLVM_BINDIR@': '$LLVM_BINDIR',
        '@SAGE@': '$SAGE',
    },
)
AddPostAction(run_script, Chmod('$TARGET', 0750))

plugin, = penv.SharedLibrary(
    'CArrayIntrospection',
    (
        'BacktrackPhiNodes.cc',
        'FindLengthChecks.cc',
        'FindSentinelHelper.cc',
        'FindStructElements.cc',
        'IIGlueReader.cc',
        'LengthAnnotator.cc',
        'NameCompare.cc',
        'NullAnnotator.cc',
        'NullAnnotatorHelper.cc',
        'SymbolicRangeTest.cc',
    ),
    LIBS=sra_plugin,
)

env['plugin'] = plugin

Alias('plugin', plugin)


########################################################################
#
#  IDE support files
#

# compilation database for use with various Clang LibTooling tools

import json

def compilation_database(env, topdir):
    for obj in plugin.sources:
        src, = obj.sources
        yield {
            'directory': topdir,
            'file': src.path,
            'command': env.subst('$SHCXXCOM', target=obj, source=src),
        }

def stash_compile_commands(target, source, env):
    sconstruct, topdir = source
    target, = target
    commands = list(compilation_database(env, topdir.read()))
    json.dump(commands, open(str(target), 'w'), indent=2)

penv.Command('compile_commands.json', ('SConstruct', Value(Dir('#').abspath)), stash_compile_commands)


# Emacs flychecker configuration

from itertools import imap

def elispString(text):
    return '"%s"' % text.replace('"', '\\"')

def elispStringList(texts):
    return '(%s)' % ' '.join(imap(elispString, texts))

config = penv.Substfile(
    '.dir-locals.el.in',
    SUBST_DICT={
        '@CPPDEFINES@': elispStringList(penv['CPPDEFINES']),
        '@CPPPATH@': elispStringList(penv['CPPPATH']),
        '@CXX@': elispString(penv.WhereIs('$CXX')),
    },
)


########################################################################
#
#  subdirectories
#

SConscript(dirs='tests', exports='env')


# Local variables:
# flycheck-flake8rc: "scons-flake8.ini"
# End:
