#!/usr/bin/env python3

from argparse import ArgumentParser, FileType
from collections import namedtuple
from csv import DictWriter, excel
from json import load
from os import linesep
from sys import stdin, stdout


Row = namedtuple('Row', [
    'function',
    'slot',
    'fixed',
    'sentinel',
    'symbolic',
])


class Simple (excel):
    lineterminator = linesep


def main():
    parser = ArgumentParser(description='Convert array annotations from JSON into canonical CSV suitable for use with spreadsheets or automated diff tools')
    parser.add_argument('-i',  '--infile', nargs='?', type=FileType('r'), default=stdin, help='JSON file to read (default: stdin)')
    parser.add_argument('-o', '--outfile', nargs='?', type=FileType('w'), default=stdout, help='CSV file to write (default: stdout)')
    options = parser.parse_args()

    functions = {
        function['function_name']: function['arguments']
        for function in load(options.infile)['library_functions']
    }

    writer = DictWriter(options.outfile, fieldnames=Row._fields, dialect=Simple)
    writer.writeheader()

    for name in sorted(functions):
        arguments = functions[name]
        for slot, details in enumerate(arguments):
            writer.writerow(Row(
                function=name,
                slot=slot,
                fixed=details.get('fixed'),
                sentinel=details.get('sentinel'),
                symbolic=details.get('symbolic'),
            )._asdict())


if __name__ == '__main__':
    main()


# LocalWords: infile stdin outfile stdout

# Local variables:
# flycheck-flake8-maximum-line-length: 160
# End:
