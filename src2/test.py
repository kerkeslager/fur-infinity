import os
import os.path
import subprocess
import unittest

# Go to the directory of the current file so we know where we are in the filesystem
os.chdir(os.path.dirname(os.path.abspath(__file__)))

class OutputTests(unittest.TestCase):
    pass

def add_output_test(filename):
    def test(self):
        p = subprocess.Popen(
            (
                './fur',
                os.path.join('test', 'integration', filename),
            ),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        actual_stdout, actual_stderr = p.communicate()

        expected_stdout_path = os.path.join('test', 'integration', filename[:-3] + 'stdout.txt')

        if os.path.isfile(expected_stdout_path):
            with open(expected_stdout_path, 'rb') as f:
                expected_stdout = f.read()
        else:
            expected_stdout = b''

        import ipdb; ipdb.set_trace()

        self.assertEqual(expected_stdout, actual_stdout)

        expected_stderr_path = os.path.join('test', 'integration', filename[:-3] + 'stderr.txt')

        if os.path.isfile(expected_stderr_path):
            with open(expected_stderr_path, 'rb') as f:
                expected_stderr = f.read()
        else:
            expected_stderr = b''

        self.assertEqual(expected_stderr, actual_stderr)

    setattr(OutputTests, 'test_' + filename[:-4], test)

class MemoryLeakTests(unittest.TestCase):
    pass

def add_memory_leak_test(filename):
    def test(self):
        command = [
            'valgrind',
            '--tool=memcheck',
            '--leak-check=yes',
            '--show-reachable=yes',
            '--num-callers=20',
            '--track-fds=yes',
            '--error-exitcode=42',
            '-q',
            './fur',
            os.path.join('test', 'integration', filename),
        ]

        actual_return = subprocess.call(command)

        expected_return = 0
        self.assertEqual(expected_return, actual_return)

    setattr(MemoryLeakTests, 'test_' + filename[:-4], test)

filenames = (
    entry.name
    for entry in os.scandir(os.path.join('test','integration'))
    if entry.is_file()
    if entry.name.endswith('.fur')
)

for filename in filenames:
    add_output_test(filename)
    add_memory_leak_test(filename)

class ScannerTests(unittest.TestCase):
    pass

def add_scanner_test(filename):
    def test(self):
        with open(os.path.join('test','scanner',filename), 'r') as f:
            source = f.read()

        p = subprocess.Popen(
            (
                './scanner_test',
                source,
            ),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        actual_stdout, actual_stderr = p.communicate()

        expected_stdout_path = os.path.join('test', 'scanner', filename[:-len('source.txt')] + 'stdout.txt')

        if os.path.isfile(expected_stdout_path):
            with open(expected_stdout_path, 'rb') as f:
                expected_stdout = f.read()
        else:
            expected_stdout = b''

        self.assertEqual(expected_stdout, actual_stdout)

        expected_stderr_path = os.path.join('test', 'scanner', filename[:-len('source.txt')] + 'stderr.txt')

        if os.path.isfile(expected_stderr_path):
            with open(expected_stderr_path, 'rb') as f:
                expected_stderr = f.read()
        else:
            expected_stderr = b''

        self.assertEqual(expected_stderr, actual_stderr)

    setattr(ScannerTests, 'test_' + filename[:-len('.source.txt')], test)

filenames = (
    entry.name
    for entry in os.scandir(os.path.join('test','scanner'))
    if entry.is_file()
    if entry.name.endswith('.source.txt')
)

for filename in filenames:
    add_scanner_test(filename)

unittest.main()
