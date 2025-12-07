
import os
import sys
import re

def fix_file(filepath):
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            content = f.read()
    except UnicodeDecodeError:
        try:
            with open(filepath, 'r', encoding='cp932') as f: # Try SJIS/CP932 for potential Japanese comments
                content = f.read()
        except Exception as e:
            print(f"Failed to read {filepath}: {e}")
            return False

    # Replace whole word NULL with nullptr
    # We use regex \bNULL\b to avoid matching part of other words
    new_content, count = re.subn(r'\bNULL\b', 'nullptr', content)

    if count > 0:
        print(f"Replacing {count} instances in {filepath}")
        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write(new_content)
        except Exception as e:
            print(f"Failed to write {filepath}: {e}")
            return False
        return True
    return False

def main():
    import argparse
    parser = argparse.ArgumentParser(description='Replace NULL with nullptr')
    parser.add_argument('targets', nargs='+', help='Files or directories to process')
    parser.add_argument('--exclude', action='append', help='Files to exclude', default=[])
    args = parser.parse_args()

    excluded_files = set(os.path.abspath(f) for f in args.exclude)

    for target in args.targets:
        if os.path.isfile(target):
            if os.path.abspath(target) not in excluded_files:
                fix_file(target)
        elif os.path.isdir(target):
            for root, dirs, files in os.walk(target):
                for file in files:
                    full_path = os.path.abspath(os.path.join(root, file))
                    if file in [os.path.basename(e) for e in args.exclude]: # Simple mismatch check by filename too
                         continue
                    if full_path in excluded_files:
                        continue
                    
                    if file.endswith('.cpp') or file.endswith('.h') or file.endswith('.hpp') or file.endswith('.c'):
                        fix_file(full_path)

if __name__ == "__main__":
    main()
