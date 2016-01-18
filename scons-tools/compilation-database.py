# compilation database for use with various Clang LibTooling tools

from SCons.Script import Action, Builder, Dir, Value

import json


def create_json(env, roots):

    def generate_entries(env, roots):
        topdir = Dir('#').abspath
        for root in roots:
            for obj in root.sources:
                source, = obj.sources
                yield {
                    'directory': topdir,
                    'file': source.path,
                    'command': env.subst('$SHCXXCOM', target=obj, source=source),
                }

    return list(generate_entries(env, roots))


def action(target, source, env):
    target, = target
    source, = source
    json.dump(
        source.read(),
        open(str(target), 'w'),
        indent=2,
    )


def action_string(target, source, env):
    target, = target
    return env.subst("Building '%s' for sources of $COMPILATION_DATABASE_ROOTS" % target)


def emitter(target, source, env):
    env['COMPILATION_DATABASE_ROOTS'] = source
    return 'compile_commands.json', Value(create_json(env, source))


########################################################################


def generate(env):
    name = 'CompilationDatabase'
    if name in env['BUILDERS']:
        return

    env.AppendUnique(
        BUILDERS={
            name: Builder(
                action=Action(
                    action,
                    action_string,
                ),
                emitter=emitter,
            ),
        },
    )


def exists(env):
    return True
