import os
import re
import subprocess

from pick import pick

# go to root
os.chdir(os.path.join(os.path.dirname(__file__), ".."))

# Get all GPU (Windows only)
p = subprocess.run(["wmic", "path", "win32_VideoController", "get", "name", ",", "AdapterCompatibility"], capture_output=True, text=True)

# Choose the GPU
title = 'Please choose the GPU: '
options = re.split("\n", p.stdout)
options = [x for x in options if 'AdapterCompatibility' not in x]
options = list(filter(None, options))


option, index = pick(options, title, indicator='=>', default_index=0)

fullName = re.split(r"\s{3}\s*", option)[1]
print(fullName)

os.environ["GPU_TO_USE"] = fullName


solutionToLaunch = R"builddir\libnodegl\libnodegl.sln"
if not os.path.isfile(solutionToLaunch):
    input("Error: " + solutionToLaunch + " doesn't exist")
else:
    subprocess.run(solutionToLaunch, shell=True, check=True)


