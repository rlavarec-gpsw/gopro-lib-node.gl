import os
import re
import shutil
import subprocess
import sys
import tempfile


def main():
    # Ensure apitrace is available
    if shutil.which("apitrace") is not None:
        print("apitrace command not found. Please install apitrace and ensure it is in your PATH.")
        print("Installation instruction: https://github.com/apitrace/apitrace/blob/master/docs/INSTALL.markdown")
        sys.exit(1)

    # Ensure one argument is provided
    if len(sys.argv) != 2:
        print("Usage: python gl_leaks.py program")
        sys.exit(1)

    # Generate the temp file
    with tempfile.NamedTemporaryFile(suffix="_ngl.trace", delete=False) as tmpfile:
        tmpfile_path = tmpfile.name

    # Trace GPU
    try:
        subprocess.run(["apitrace", "trace", "-a", "egl", "-o", tmpfile_path] + sys.argv[1:])

        leaks = subprocess.run(
            ["apitrace", "leaks", tmpfile_path], stderr=subprocess.PIPE, stdout=subprocess.DEVNULL, text=True
        )

        if re.search("error:", leaks.stderr):
            print("GPU leaks detected: YES")
            print(leaks.stderr)
            sys.exit(1)
        else:
            print("GPU leaks detected: NO")

    finally:
        os.remove(tmpfile_path)


if __name__ == "__main__":
    main()
