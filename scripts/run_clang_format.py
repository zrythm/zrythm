# SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import subprocess
import sys
import fnmatch
import difflib

def colorize(text, color):
    colors = {
        'red': '\033[91m',
        'green': '\033[92m',
        'yellow': '\033[93m',
        'reset': '\033[0m'
    }
    return f"{colors[color]}{text}{colors['reset']}"

def get_git_tracked_files():
    result = subprocess.run(['git', 'ls-files', '*.cpp', '*.hpp', '*.c', '*.h'], 
                            capture_output=True, text=True, check=True)
    return result.stdout.splitlines()

def read_ignore_patterns(ignore_file):
    with open(ignore_file, 'r', encoding='utf-8') as f:
        return [line.strip() for line in f if line.strip() and not line.startswith('#')]

def should_ignore(file_path, ignore_patterns):
    for pattern in ignore_patterns:
        if pattern.endswith('/'):
            pattern = pattern + '**'
        if fnmatch.fnmatch(file_path, pattern):
            return True
    return False

def run_clang_format(dry_run):
    ignore_patterns = read_ignore_patterns('.clang-format-ignore')
    files = get_git_tracked_files()
    
    formatting_issues_found = False
    
    for file in files:
        if not should_ignore(file, ignore_patterns):
            
            with open(file, 'r', encoding='utf-8') as f:
                original = f.read()
            
            formatted = subprocess.run(['clang-format', file], 
                                       capture_output=True, text=True).stdout
            
            if original != formatted:
                formatting_issues_found = True
                print(f"Formatting issues in {file}:")
                diff = difflib.unified_diff(
                    original.splitlines(keepends=True),
                    formatted.splitlines(keepends=True),
                    fromfile=file,
                    tofile=f"{file} (formatted)"
                )
                for line in diff:
                    if line.startswith('+'):
                        sys.stdout.write(colorize(line, 'green'))
                    elif line.startswith('-'):
                        sys.stdout.write(colorize(line, 'red'))
                    else:
                        sys.stdout.write(line)
                
            
                if not dry_run:
                    with open(file, 'w', encoding='utf-8') as f:
                        f.write(formatted)
    
    if dry_run and formatting_issues_found:
        sys.exit(1)

if __name__ == '__main__':
    dry_run = '--dry-run' in sys.argv
    run_clang_format(dry_run)
