from SCons.Script import Action, Builder, Touch

import filecmp


########################################################################
#
#  comparison with reference output
#


def __compare_action_exec(target, source, env):
    [actual, expected] = source
    return not filecmp.cmp(str(actual), str(expected), False)


def __compare_action_show(target, source, env):
    [actual, expected] = source
    return 'compare "%s" and "%s"' % (actual, expected)


__compare_action = Action(__compare_action_exec, __compare_action_show)


__expect_action = [__compare_action, Touch('$TARGET')]


def __expect_emitter(target, source, env):
    [actual] = source
    expected = actual.target_from_source('', '.expected')
    return target, [actual, expected]


__expect_builder = Builder(
    action=__expect_action,
    emitter=__expect_emitter,
    suffix='.passed',
)


########################################################################


def generate(env):
    env.AppendUnique(
        BUILDERS={
            'Expect': __expect_builder,
        },
    )


def exists(env):
    return True
