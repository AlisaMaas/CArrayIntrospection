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

variables = Variables(['.scons-options'], ARGUMENTS)
variables.Add(PathVariable('IIGLUE', 'Path to iiglue executable', '/p/polyglot/public/iiglue-tools/bin/iiglue', pathIsExecutable))


########################################################################
#
#  common basis for all build environments
#

env = Environment(
    LLVM_ROOT=Dir('/p/polyglot/public/llvm/install'),
    tools=(
        'bitcode',
        'default',
        'expect',
        'iiglue',
    ),
    toolpath=(
        'scons-tools',
    ),
    variables=variables,
)

Help(variables.GenerateHelpText(env))

env.PrependENVPath('PATH', env.subst('$LLVM_ROOT/bin'))


########################################################################
#
#  build environment for compiling LLVM plugins
#

penv = env.Clone(
    CXXFLAGS=('-Wall', '-Wextra', '-Werror', '-std=c++11'),
    CPPPATH='/unsup/boost-1.55.0/include',
    INCPREFIX='-isystem ',
    LIBS=('LLVM-3.4',),
)

penv.PrependENVPath('PATH', '/s/gcc-4.9.0/bin')
penv.ParseConfig('llvm-config --cxxflags --ldflags')
penv.AppendUnique(
    CCFLAGS=(
        '-fexceptions',
        '-frtti',
    ), delete_existing=True)

plugin, = penv.SharedLibrary('CArrayIntrospection', (
    'IIGlueReader.cc',
    'FindSentinels.cc',
    'NullAnnotator.cc',
))

env['plugin'] = plugin


########################################################################
#
#  subdirectories
#

SConscript(dirs='tests', exports='env')


# Local variables:
# flycheck-flake8rc: "scons-flake8.ini"
# End:
