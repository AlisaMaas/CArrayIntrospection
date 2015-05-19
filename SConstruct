########################################################################
#
#  path to Tristan Ravitch's iiglue analyzer

from os import access, environ, X_OK
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

variables = Variables(['.scons-options'], ARGUMENTS)
variables.Add(PathVariable('IIGLUE', 'Path to iiglue executable', '/p/polyglot/public/bin/iiglue', pathIsOptionalExecutable))

default = WhereIs('llvm-config', (
	'/home/ajmaas/sra/llvm-3.5.1.src/Release+Asserts/bin/',
    '/p/polyglot/public/bin',
    '/usr/bin',
))
variables.Add(PathVariable('LLVM_CONFIG', 'Path to llvm-config executable', default, pathIsExecutable))

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
    CXXFLAGS=('-Wall', '-Wextra', '-Werror', '-std=c++11', '-fPIC'),
    CPPPATH=('/home/ajmaas/sra/llvm-3.5.1.src/Release+Asserts/lib','/unsup/boost-1.55.0/include', '/home/ajmaas/sra/llvm-3.5.1.src/lib/Transforms/llvm-sra/', '/usr/include/python2.7/',),
    INCPREFIX='-isystem ',
    LIBS=('LLVM-$llvm_version',),
)

sraFlags = [x for x in penv['CXXFLAGS'] if x[:2] != '-W']

sraObject = penv.SharedObject(['SymbolicRangeTest.cc', 'FindLengthChecks.cc'], CXXFLAGS=(sraFlags, '-fpermissive'))


penv.PrependENVPath('PATH', '/s/gcc-4.9.0/bin')
penv.ParseConfig('$LLVM_CONFIG --cxxflags --ldflags')
penv.AppendUnique(
    CCFLAGS=(
        '-fexceptions',
        '-frtti',
    ), delete_existing=True)

plugin, = penv.SharedLibrary('CArrayIntrospection', (
    'BacktrackPhiNodes.cc',
    'IIGlueReader.cc',
    'NullAnnotatorHelper.cc',
    'NullArgumentAnnotator.cc',
    sraObject,
    'LengthAnnotator.cc',
    'FindSentinelHelper.cc',
    'FindStructElements.cc',
    'NullElementAnnotator.cc',
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
