########################################################################
#
# performance boosters
#

CacheDir('.scons-cache')
SetOption('implicit_cache', True)
SetOption('max_drift', 1)


########################################################################
#
#  configurable paths to various supporting tools
#

from os import access, R_OK, X_OK


def pathIsExecutable(key, val, env):
    found = env.WhereIs(val)
    if found:
        val = found
    PathVariable.PathIsFile(key, val, env)
    if not access(val, X_OK):
        raise SCons.Errors.UserError('Path for option %s is not executable: %s' % (key, val))


def pathIsOptionalExecutable(key, val, env):
    if val:
        pathIsExecutable(key, val, env)


variables = Variables(['.scons-options'], ARGUMENTS)
variables.Add(BoolVariable('DEBUG', 'compile for debugging', False))
variables.Add(PathVariable('IIGLUE', 'Path to iiglue executable', '/p/polyglot/public/bin/iiglue', pathIsOptionalExecutable))
variables.Add(BoolVariable('VALGRIND', 'run tests under Valgrind', False))

llvmConfigDefault = WhereIs('llvm-config', (
    '/p/polyglot/public/tools/llvm-3.7.1/install/bin',
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
        '/unsup/boost/include',
    ),
    INCPREFIX='-isystem ',
    LIBS=('LLVM-$llvm_version',),
    SHLIBPREFIX=None,
    tools=(
        'compilation-database',
        'dir-locals-el',
    ),
)
penv.ParseConfig('$LLVM_CONFIG --cxxflags --ldflags')
penv.AppendUnique(
    CCFLAGS=(
        '-fexceptions',
        '-frtti',
        '-Wall',
        '-Wextra',
        '-Werror',
        '${("", "-g")[DEBUG]}',
    ),
    delete_existing=True
)

if penv['DEBUG']:
    try:
        penv['CPPDEFINES'].remove('NDEBUG')
    except ValueError:
        pass

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

subst_dict = {
    '@LLVM_BINDIR@': '$LLVM_BINDIR',
    '@SAGE@': '$SAGE',
    '@VALGRIND@': '${ "valgrind --error-exitcode=166 --leak-check=full --errors-for-leak-kinds=definite --suppressions=python.supp --suppressions=sage.supp" if VALGRIND else "" }'
}

run_script = penv.Substfile('run.in', SUBST_DICT=subst_dict)
expanded_dict = {key: env.subst(value) for key, value in subst_dict.iteritems()}
env.Depends(run_script, Value(expanded_dict))
AddPostAction(run_script, Chmod('$TARGET', 0750))

plugin, = penv.SharedLibrary(
    'CArrayIntrospection',
    (
        'Annotator.cc',
        'AnnotatorHelper.cc',
        'BacktrackPhiNodes.cc',
        'CheckGetElementPtrVisitor.cc',
        'FindLengthChecks.cc',
        'FindLengthLoops.cc',
        'FindSentinelHelper.cc',
        'FindStructElements.cc',
        'IIGlueReader.cc',
        'LengthInfo.cc',
        'NameCompare.cc',
        'NoPointerArithmetic.cc',
        'NoPointerComparisons.cc',
        'StructElement.cc',
        'SymbolicRangeTest.cc',
        'ValueReachesValue.cc',
        'ValueSetSet.cc',
    ),
    LIBS=sra_plugin,
)

env['plugin'] = plugin

Alias('plugin', plugin)


########################################################################
#
#  IDE support files
#

penv.CompilationDatabase(plugin)
penv.DirLocalsEl()


########################################################################
#
#  subdirectories
#

SConscript(dirs='tests', exports='env')


# Local variables:
# flycheck-flake8rc: "scons-flake8.ini"
# End:
