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
    check(X_OK, 'not executable', 'make')
    check(X_OK, 'not executable', 'llvm-3.5.1.src/Release+Asserts/bin/llvm-config')
    check(R_OK, 'missing or unreadable', 'llvm-3.5.1.src/lib/Transforms/llvm-sra/SymbolicRangeAnalysis.h')
    check(R_OK, 'missing or unreadable', 'llvm-3.5.1.src/Release+Asserts/lib/SRA.so')

    if problems:
        problems = '\n\t'.join(problems)
        raise SCons.Errors.UserError('Invalid path for option %s:\n\t%s\n' % (key, problems))

variables = Variables(['.scons-options'], ARGUMENTS)
variables.Add(PathVariable('IIGLUE', 'Path to iiglue executable', '/p/polyglot/public/bin/iiglue', pathIsOptionalExecutable))
variables.Add(PathVariable('LLVM_SRA', 'Path to root of llvm-sra build tree', '/p/polyglot/public/tools/llvm-sra', pathIsSRA))


########################################################################
#
#  common basis for all build environments
#

env = Environment(
    LLVM_CONFIG='$LLVM_SRA/llvm-3.5.1.src/Release+Asserts/bin/llvm-config',
    tools=(
        'default',              # load first, so others can override
        'bitcode',
        'clang-analyzer',
        'expect',
        'iiglue',
        'plugin',
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
    CXXFLAGS=('-std=c++11', '-fPIC'),
    CPPPATH=(
        '$LLVM_SRA/llvm-3.5.1.src/lib/Transforms/llvm-sra',
        '/unsup/boost-1.55.0/include',
        '/usr/include/python2.7',
    ),
    INCPREFIX='-isystem ',
    LIBS=('LLVM-$llvm_version',),
)

sraEnv = penv.Clone()
sraEnv.AppendUnique(CXXFLAGS=('-fpermissive'))
sraObjects = sraEnv.SharedObject(['SymbolicRangeTest.cc', 'FindLengthChecks.cc'])

penv.PrependENVPath('PATH', '/s/gcc-4.9.0/bin')
penv.ParseConfig('$LLVM_CONFIG --cxxflags --ldflags')
penv.AppendUnique(
    CCFLAGS=('-fexceptions', '-frtti'),
    CXXFLAGS=('-Wall', '-Wextra', '-Werror'),
    delete_existing=True
)

plugin, = penv.SharedLibrary('CArrayIntrospection', (
    'BacktrackPhiNodes.cc',
    'IIGlueReader.cc',
    'NullAnnotatorHelper.cc',
    'NullAnnotator.cc',
    'LengthAnnotator.cc',
    'FindSentinelHelper.cc',
    'FindStructElements.cc',
    sraObjects,
))

env['plugin'] = plugin

Alias('plugin', plugin)


########################################################################
#
#  compilation database for use with various Clang LibTooling tools
#


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


########################################################################
#
#  subdirectories
#

SConscript(dirs='tests', exports='env')


# Local variables:
# flycheck-flake8rc: "scons-flake8.ini"
# End:
