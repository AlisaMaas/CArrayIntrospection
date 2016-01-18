from itertools import imap


def elispString(text):
    return '"%s"' % text.replace('"', '\\"')


def elispStringList(texts):
    return '(%s)' % ' '.join(imap(elispString, texts))


def DirLocalsEl(self):
    return self.Substfile(
        '.dir-locals.el.in',
        SUBST_DICT={
            '@CPPDEFINES@': elispStringList(self['CPPDEFINES']),
            '@CPPPATH@': elispStringList(self['CPPPATH']),
            '@CXX@': elispString(self.WhereIs('$CXX')),
        },
    )


########################################################################


def generate(env):
    env.AddMethod(DirLocalsEl)


def exists(env):
    return True
