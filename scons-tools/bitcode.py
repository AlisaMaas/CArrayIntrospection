from SCons.Script import Builder, CScanner


########################################################################
#
#  generate LLVM bitcode from C/C++ source using the Clang front end
#


def __bitcode_builder(stage, suffix):
    cxx_action = '$__CLANG_COMMAND_CXX ' + stage
    cxx_suffixes = ['cc', 'cpp', 'cxx']

    actions = {'c': '$__CLANG_COMMAND_C ' + stage}
    for src_suffix in cxx_suffixes:
        actions[src_suffix] = cxx_action

    return Builder(
        action=actions,
        src_suffix=['c'] + cxx_suffixes,
        suffix=suffix,
        source_scanner=CScanner,
    )


########################################################################


def generate(env):
    if '__CLANG_COMMON_FLAGS' in env:
        return

    env.AppendUnique(
        CLANG_C  ='clang',
        CLANG_CXX='clang++',
        __CLANG_COMMON_FLAGS='-emit-llvm -o $TARGET $SOURCES $CCFLAGS $_CCCOMCOM $CLANG_FLAGS',
        __CLANG_COMMAND_C  ='$CLANG_C   $__CLANG_COMMON_FLAGS $CFLAGS',
        __CLANG_COMMAND_CXX='$CLANG_CXX $__CLANG_COMMON_FLAGS $CXXFLAGS',
        BUILDERS={
            'BitcodeBinary': __bitcode_builder('-c', 'bc'),
            'BitcodeSource': __bitcode_builder('-S', 'll'),
        },
    )


def exists(env):
    return env.WhereIs('clang')
