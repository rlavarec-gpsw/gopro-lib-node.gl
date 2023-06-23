
import glob
import os
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET

from pick import pick

# go to root
repoDir = os.path.join(os.path.dirname(__file__), "..")
os.chdir(repoDir)

ET.register_namespace("", "http://schemas.microsoft.com/developer/msbuild/2003")


# Choose the GPU
title = 'Please choose the Backend: '

options = ["d3d12", "vulkan", "opengl", "opengles"]
option, index = pick(options, title, indicator='=>', default_index=0)

backend = option

buildir = os.path.join("builddir", "libnodegl")


# deploy inside the nodegl dir as isolated
deployDir = R"$(ProjectDir)\..\..\venv\Scripts"

# check inside the bundle buildir and deply inside the scripts dir
if os.path.exists(os.path.join(repoDir, "stupeflix", "sripts")):
    deployDir = R"$(ProjectDir)\..\..\..\..\stupeflix\Scripts"

# check inside the bundle buildir and deply inside the scripts dir
if os.path.exists(os.path.join(repoDir, "..", ".sx", "stupeflix", "Scripts")):
    deployDir = R"$(ProjectDir)\..\..\..\.sx\stupeflix\Scripts"

deployDir += "\\"

def getFilename( filename ):
    return os.path.join(buildir, filename)

libnodegl_sln = "libnodegl.sln"

nodegl_vcxproj = "nodegl@sha.vcxproj"

listProjectToInclude = {"ngl-python": "..\\ngl-tools\\ngl-python@exe.vcxproj",
                        "ngl-desktop": "..\\ngl-tools\\ngl-desktop@exe.vcxproj",
                        "ngl-serialize": "..\\ngl-tools\\ngl-serialize@exe.vcxproj"}

listProjectNodeGLLib = {"nodegl": nodegl_vcxproj}

listBuiltType = []

'''
for file in glob.glob("builddir/**/*.vcxproj"):
    with open(file, 'r') as f:
        project_vcxproj_data_file = "\n".join(f.readlines())
    project_vcxproj_data_file = project_vcxproj_data_file.replace("debug|", "Debug|")
    project_vcxproj_data_file = project_vcxproj_data_file.replace("release|", "Release|")
    project_vcxproj_data_file = project_vcxproj_data_file.replace(">debug<", ">Debug<")
    project_vcxproj_data_file = project_vcxproj_data_file.replace(">release<", ">Release<")

    with open(file, 'w') as f:
        f.write(project_vcxproj_data_file)
'''

with open(getFilename(libnodegl_sln), 'r') as f:
    nodegl_sln_data_file = "".join(f.readlines())
    projectID = re.search("Project\(\"{(.*)}\"\)", nodegl_sln_data_file).group(1)
    '''
    nodegl_sln_data_file = nodegl_sln_data_file.replace("debug|", "Debug|")
    nodegl_sln_data_file = nodegl_sln_data_file.replace("release|", "Release|")
    '''
    # Is the solution got release inside
    if nodegl_sln_data_file.find("release") != -1:
        listBuiltType.append("release")

    # Is the solution got debug inside
    if nodegl_sln_data_file.find("debug") != -1:
        listBuiltType.append("debug")

    # Must have found release or debug
    assert len(listBuiltType) >= 1
    print("Setup build type -> ", listBuiltType)

    AllProjectEntry = ''
    AllProjectConfig = ''
    for name, project in listProjectToInclude.items():
        with open(getFilename(project), 'r') as f:
            project_vcxproj_data_file = "\n".join(f.readlines())
            project_uid = re.search("ProjectGuid>{(.*?)}", project_vcxproj_data_file).group(1)

            # If the project doesn't exist yet in sln
            if nodegl_sln_data_file.find(f'"{name}"') == -1:
                AllProjectEntry += 'Project("{'+projectID+'}") = "'+name+'", "'+project+'", "{'+project_uid+'}"' + "\n" + 'EndProject' + "\n"
                for builtType in listBuiltType:
                    AllProjectConfig += '{'+project_uid+'}.'+builtType+'|x64.ActiveCfg = '+builtType+'|x64' + "\n" + '{'+project_uid+'}.'+builtType+'|x64.Build.0 = '+builtType+'|x64' + "\n"

    nodegl_sln_data_file = nodegl_sln_data_file.replace("EndProject\nGlobal\n", 'EndProject\n' + AllProjectEntry + 'Global' + "\n")
    for builtType in listBuiltType:
        nodegl_sln_data_file = nodegl_sln_data_file.replace("ActiveCfg = "+builtType+"|x64\n	EndGlobalSection\n", "ActiveCfg = "+builtType+"|x64\n	" + AllProjectConfig + "EndGlobalSection" + "\n")

with open(getFilename(libnodegl_sln), 'w') as f:
    f.write(nodegl_sln_data_file)

def getOrCreate(xmlObj, name, findSubElement=None, findAttribute=None):
    tXmlObj = xmlObj.findall('{*}' + name)
    if len(tXmlObj) == 0:
        return ET.SubElement(xmlObj, name)

    for elem in tXmlObj:
        if findSubElement:
            tSubElement = elem.findall('{*}' + findSubElement)

            if len(tSubElement) == 0:
                continue

            if findAttribute:
                for subElem in tSubElement:
                    if findAttribute and (subElem.attrib[findAttribute[0]] == findAttribute[1]):
                        return elem
            else:
                return elem

    # return new item if not found
    if findSubElement or findAttribute:
        return ET.SubElement(xmlObj, name)

    # Return first item if found
    return tXmlObj[0]

def getVCXProjXml(filename):
    return ET.parse(filename)  # Must have a vcxproj

def updateProject(projectName, commands):

    if projectName in listProjectToInclude:
        projectUserFile = getFilename(listProjectToInclude[projectName])
    else:
        projectUserFile = getFilename(listProjectNodeGLLib[projectName])

    tree = getVCXProjXml(projectUserFile)
    ProjectXml = tree.getroot()

    for builtType in listBuiltType:
        ItemDefinitionGroupXML = getOrCreate(ProjectXml, 'ItemDefinitionGroup')
        CommandXml = getOrCreate(getOrCreate(ItemDefinitionGroupXML, "CustomBuildStep", "Command", ["Condition", "'$(Configuration)|$(Platform)'=='"+builtType+"|x64'"]), 'Command')
        CommandXml.set("Condition", "'$(Configuration)|$(Platform)'=='"+builtType+"|x64'")
        CommandXml.text = commands
        OutputsXml = getOrCreate(getOrCreate(ItemDefinitionGroupXML, "CustomBuildStep", "Command", ["Condition", "'$(Configuration)|$(Platform)'=='"+builtType+"|x64'"]), 'Outputs')
        OutputsXml.set("Condition", "'$(Configuration)|$(Platform)'=='"+builtType+"|x64'")
        OutputsXml.text = R'dummy_file.txt'


    ET.indent(tree, space="\t", level=0)
    tree.write(projectUserFile)

def getVCXProjUserXml(filename):
    if os.path.isfile(filename):
        return ET.parse(filename)  # parse an open file
    root = ET.Element('Project')
    root.set("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003")
    root.set("ToolsVersion", "Current")

    for builtType in listBuiltType:
        PropertyGroupXml = ET.SubElement(root, "PropertyGroup")
        PropertyGroupXml.set("Condition", "'$(Configuration)|$(Platform)'=='"+builtType+"|x64'")
    return ET.ElementTree(root)


def updateProjectUser(projectName, command):
    projectUserFile = getFilename(listProjectToInclude[projectName] + ".user")

    tree = getVCXProjUserXml(projectUserFile)
    ProjectXml = tree.getroot()

    for PropertyGroupXML in ProjectXml.findall('{*}PropertyGroup'):
        getOrCreate(PropertyGroupXML, "LocalDebuggerCommandArguments").text = command
        getOrCreate(PropertyGroupXML, "LocalDebuggerCommand").text = os.path.join(deployDir, "$(TargetFileName)")
        getOrCreate(PropertyGroupXML, "LocalDebuggerWorkingDirectory").text = deployDir
        getOrCreate(PropertyGroupXML, "DebuggerFlavor").text = R"WindowsLocalDebugger"

    ET.indent(tree, space="\t", level=0)
    tree.write(projectUserFile)


updateProject("nodegl", f'copy /Y "$(TargetPath)" "{deployDir}"' + "\n" + R'copy /Y "$(TargetPath)" "$(ProjectDir)\..\..\pynodegl\"')
updateProject("ngl-python", f'copy /Y "$(TargetPath)" "{deployDir}"' + "\n" + R'rmdir /S /Q "$(temp)\ngl-desktop\localhost-1234"')
updateProject("ngl-desktop", f'copy /Y "$(TargetPath)" "{deployDir}"' + "\n" + R'rmdir /S /Q "$(temp)\ngl-desktop\localhost-1234"')
updateProject("ngl-serialize", f'copy /Y "$(TargetPath)" "{deployDir}"' + "\n" + R'rmdir /S /Q "$(temp)\ngl-desktop\localhost-1234"')

listProjectToInclude = listProjectToInclude | listProjectNodeGLLib
updateProjectUser("nodegl", "")
updateProjectUser("ngl-python", "-b "+backend+R" $(ProjectDir)\..\..\builddir\tests\..\..\tests\compute.py compute_cubemap_load_store")
updateProjectUser("ngl-desktop", "-b "+backend)
updateProjectUser("ngl-serialize", R"$(ProjectDir)\..\..\builddir\tests\..\..\tests\compute.py compute_cubemap_load_store c:\tmp\compute_cubemap_load_store.ngl")

print("setup dev sln -> done")

subprocess.call([sys.executable, os.path.join(repoDir, "scripts", "launch_dev_sln.py")])