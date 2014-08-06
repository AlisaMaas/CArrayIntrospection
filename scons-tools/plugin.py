from SCons.Script import *


########################################################################
#
#  run our analysis plugin and collect results
#


def __run_plugin_emitter(target, source, env):
    bitcode, json = source
    source.append('$plugin')
    return target, source


__run_plugin_builder = Builder(
    action='opt -analyze -o $TARGET -load ./${SOURCES[2]} -iiglue-read-file ${SOURCES[1]} $PLUGIN_ARGS ${SOURCES[0]}',
    emitter=__run_plugin_emitter,
    suffix='.actual',
)


########################################################################


def generate(env):
    if 'RunPlugin' in env['BUILDERS']:
        return

    env.AppendUnique(
        BUILDERS={
            'RunPlugin': __run_plugin_builder,
        },
        jsondir='.',
    )


def exists(env):
    return true
