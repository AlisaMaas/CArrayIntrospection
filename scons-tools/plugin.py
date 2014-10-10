from SCons.Script import *
from SCons.Util import splitext


########################################################################
#
#  run our analysis plugin and collect results
#


def __run_plugin_emitter(target, source, env):
    source.insert(0, '$plugin')
    return target, source


def __run_plugin_source_args(target, source, env, for_signature):
    def generate():
        for input in source:
            extension = splitext(input.name)[1]
            if extension == '.json':
                yield '-iiglue-read-file'
                yield input
            elif extension == '.so':
                yield '-load'
                yield './%s' % input
            else:
                yield input
    return list(generate())


__run_plugin_builder = Builder(
    action='opt -analyze -o $TARGET $_RUN_PLUGIN_SOURCE_ARGS $PLUGIN_ARGS',
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
        _RUN_PLUGIN_SOURCE_ARGS=__run_plugin_source_args,
        jsondir='.',
    )


def exists(env):
    return true
