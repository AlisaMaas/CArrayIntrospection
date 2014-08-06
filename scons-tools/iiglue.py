from SCons.Script import *


########################################################################
#
#  generate JSON analysis output from LLVM bitcode
#

def __iiglue_analyze_builder(env):
    return Builder(
        action='$IIGLUE $SOURCE -r $TARGET.dir',
        suffix='.json',
        src_suffix=['.bc', '.ll'],
        src_builder=env['BUILDERS'].get('BitcodeSource'),
    )


########################################################################


def generate(env):
    if 'IIGlueAnalyze' in env['BUILDERS']:
        return

    env.AppendUnique(
        IIGLUE='iiglue',
        BUILDERS={
            'IIGlueAnalyze': __iiglue_analyze_builder(env),
        },
    )


def exists(env):
    return true
