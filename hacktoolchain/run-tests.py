#!/usr/bin/python3

import difflib
import os
import re
import subprocess
import sys
import tempfile

line_no = re.compile('^\s*[\da-f]+:', re.M)

class Tester:
    def __init__(self):
        # Occupy some temporary file names
        self.temp_obj = tempfile.NamedTemporaryFile()

        # Configurations
        self.old_as = 'as'
        self.new_as = './as'

    def assemble_and_objdump(self, define, assembler, filename):
        '''Assemble and objdump. Return result or error message.'''
        preproc = subprocess.Popen(['cpp', define, filename], stdout=subprocess.PIPE)
        assemble_cmdline = [assembler, '-o', self.temp_obj.name, '-']
        if 'x32' in filename:
            assemble_cmdline.append('--x32')
        assemble = subprocess.Popen(assemble_cmdline, stdin=preproc.stdout)
        if preproc.wait() != 0 or assemble.wait() != 0:
            return 'Failed to assemble {} ({})'.format(filename, define)

        # Objdump
        objdump_proc = subprocess.Popen(['objdump', '-d', '-r', self.temp_obj.name], stdout=subprocess.PIPE)
        objdump = objdump_proc.stdout.read().decode('ascii')
        if objdump_proc.wait() != 0:
            return 'Failed to objdump {} ({})'.format(filename, define)

        # Remove address
        objdump = line_no.sub('', objdump)

        return objdump

    def single_test(self, filename):
        '''Run one test.
        Output result to stdout.
        '''

        old_objdump = self.assemble_and_objdump('-DORIGINAL=0', self.old_as, filename)
        new_objdump = self.assemble_and_objdump('-DORIGINAL=1', self.new_as, filename)

        if old_objdump == new_objdump:
            print('\033[34m{} PASSED.\033[0m'.format(filename))

        else:
            diff_gen = difflib.unified_diff(new_objdump.splitlines(), old_objdump.splitlines(),
                    n = 7, fromfile=filename + ' ACTUAL', tofile=filename + ' EXPECTED')
            for diff_line in diff_gen:
                diff_line = diff_line.rstrip()
                if diff_line.startswith('-'):
                    print('\033[31m{}\033[0m'.format(diff_line))
                elif diff_line.startswith('+'):
                    print('\033[34m{}\033[0m'.format(diff_line))
                elif diff_line.startswith('@@'):
                    print('\033[32m{}\033[0m'.format(diff_line))
                else:
                    print(diff_line)

    def test_many(self, filenames):
        for f in filenames:
            self.single_test(f)

def main():
    tester = Tester()
    if len(sys.argv) > 1:
        filenames = sys.argv[1:]
    else:
        filenames = sorted(os.path.join('tests', f) for f in os.listdir('tests') if f.endswith('.S'))
    tester.test_many(filenames)

if __name__ == '__main__':
    main()
