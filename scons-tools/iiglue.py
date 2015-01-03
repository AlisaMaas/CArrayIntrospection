from itertools import imap
from SCons.Script import *


########################################################################
#
#  generate JSON analysis output from LLVM bitcode
#


def __iiglue_target_scanner(node, env, path):
    iiglue = env.File('$IIGLUE')
    if iiglue.path == '/p/polyglot/public/bin/iiglue':
        return (
            '/p/polyglot/public/iiglue-tools/.cabal-sandbox/bin/iiglue',
            '/p/polyglot/public/tools/z3-4.3.2.5a45711f22d9-x64-debian-7.4/bin/z3',
            '/unsup/llvm-3.3/bin/opt',
        )
    else:
        return filter(bool, imap(env.WhereIs, ('opt', 'z3')))


def __iiglue_analyze_builder(env):
    return Builder(
        action='$IIGLUE $SOURCE -r $TARGET.dir 2>/dev/null',
        suffix='.json',
        src_suffix=['.bc', '.ll'],
        src_builder=env['BUILDERS'].get('BitcodeSource'),
        target_scanner=Scanner(__iiglue_target_scanner),
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
