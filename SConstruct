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
    ),
    toolpath=(
        'scons-tools',
    ),
)

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

plugin = penv.SharedLibrary('IIGlueReader.cc')
Default(plugin)


########################################################################
#
#  subdirectories
#

SConscript(dirs='tests', exports=['env', 'penv'])


# Local variables:
# flycheck-flake8rc: "scons-flake8.ini"
# End:
