# Import and propagate environment variables to do the right thing if
# scons is run under the Clang Static Analyzer, as in:
#
#   % scan-build [scan-build options] scons [scons options]
#
# Based on <http://stackoverflow.com/a/9305378/91929>.


def generate(env):
    from os import environ
    if 'CCC_ANALYZER_OUTPUT_FORMAT' in environ:
        env['CC'] = environ['CC']
        env['CXX'] = environ['CXX']
        env['ENV'].update(item for item in environ.iteritems() if item[0].startswith('CCC_'))


def exists(env):
    return True
