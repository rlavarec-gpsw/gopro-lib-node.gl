
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

for file in glob.glob("builddir/**/*.sln"):
    with open(file, 'r') as f:
        project_vcxproj_data_file = "".join(f.readlines())
    project_vcxproj_data_file = project_vcxproj_data_file.replace("debug|", "Debug|")
    project_vcxproj_data_file = project_vcxproj_data_file.replace("release|", "Release|")
    project_vcxproj_data_file = project_vcxproj_data_file.replace(">debug<", ">Debug<")
    project_vcxproj_data_file = project_vcxproj_data_file.replace(">release<", ">Release<")

    with open(file, 'w') as f:
        f.write(project_vcxproj_data_file)

tListVcxproj = {}
for file in glob.glob("builddir/**/*.vcxproj"):
    tListVcxproj[file] = file
    with open(file, 'r') as f:
        project_vcxproj_data_file = "".join(f.readlines())
    project_vcxproj_data_file = project_vcxproj_data_file.replace("debug|", "Debug|")
    project_vcxproj_data_file = project_vcxproj_data_file.replace("release|", "Release|")
    project_vcxproj_data_file = project_vcxproj_data_file.replace(">debug<", ">Debug<")
    project_vcxproj_data_file = project_vcxproj_data_file.replace(">release<", ">Release<")

    with open(file, 'w') as f:
        f.write(project_vcxproj_data_file)

tIsReleaseAddDebug = False
with open(getFilename(libnodegl_sln), 'r') as f:
    nodegl_sln_data_file = "".join(f.readlines())
    projectID = re.search("Project\(\"{(.*)}\"\)", nodegl_sln_data_file).group(1)

    # Is the solution got release inside
    if nodegl_sln_data_file.find("Release") != -1:
        listBuiltType.append("Release")
        tIsReleaseAddDebug = True
        listBuiltType.append("Debug")

    # Is the solution got debug inside
    if nodegl_sln_data_file.find("Debug") != -1:
        if tIsReleaseAddDebug:
            tIsReleaseAddDebug = False
        else:
            listBuiltType.append("Debug")

    if tIsReleaseAddDebug:
        SolutionConfigurationPlatforms = re.search(r"GlobalSection\(SolutionConfigurationPlatforms\) = preSolution([\w\W]*?)EndGlobalSection", nodegl_sln_data_file).group(1)
        SolutionConfigurationPlatforms = SolutionConfigurationPlatforms + "\n" + SolutionConfigurationPlatforms.replace("Release", "Debug")
        nodegl_sln_data_file = re.sub(r"GlobalSection\(SolutionConfigurationPlatforms\) = preSolution([\w\W]*?)EndGlobalSection", "GlobalSection(SolutionConfigurationPlatforms) = preSolution" + SolutionConfigurationPlatforms + "EndGlobalSection", nodegl_sln_data_file)

        ProjectConfigurationPlatforms = re.search(r"GlobalSection\(ProjectConfigurationPlatforms\) = postSolution([\w\W]*?)EndGlobalSection", nodegl_sln_data_file).group(1)
        ProjectConfigurationPlatforms = ProjectConfigurationPlatforms + "\n" + ProjectConfigurationPlatforms.replace("Release", "Debug")
        nodegl_sln_data_file = re.sub(r"GlobalSection\(ProjectConfigurationPlatforms\) = postSolution([\w\W]*?)EndGlobalSection", "GlobalSection(ProjectConfigurationPlatforms) = postSolution" + ProjectConfigurationPlatforms + "EndGlobalSection", nodegl_sln_data_file)


    # Must have found release or debug
    #assert len(listBuiltType) >= 1
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
                    if findAttribute and (findAttribute[0] in subElem.attrib) and (subElem.attrib[findAttribute[0]] == findAttribute[1]):
                        return elem
            else:
                return elem
        else:

            if findAttribute:
                if (findAttribute[0] in elem.attrib) and (elem.attrib[findAttribute[0]] == findAttribute[1]):
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
        projectFile = getFilename(listProjectToInclude[projectName])
    elif projectName in listProjectNodeGLLib:
        projectFile = getFilename(listProjectNodeGLLib[projectName])
    else:
        projectFile = projectName

    tree = getVCXProjXml(projectFile)
    ProjectXml = tree.getroot()

    for builtType in listBuiltType:
        ItemGroupXML = getOrCreate(ProjectXml, 'ItemGroup', None,  ["Label", '"ProjectConfigurations"'])
        ProjectConfigurationXML = getOrCreate(ItemGroupXML, 'ProjectConfiguration', None, ["Include", builtType+"|x64"])
        ProjectConfigurationXML.set("Include", builtType+"|x64")
        ConfigurationXML = getOrCreate(ProjectConfigurationXML, 'Configuration')
        ConfigurationXML.text = builtType
        PlatformXML = getOrCreate(ProjectConfigurationXML, 'Platform')
        PlatformXML.text = 'x64'

        if commands != '':
            ItemDefinitionGroupXML = getOrCreate(ProjectXml, 'ItemDefinitionGroup')
            CommandXml = getOrCreate(getOrCreate(ItemDefinitionGroupXML, "CustomBuildStep", "Command", ["Condition", "'$(Configuration)|$(Platform)'=='"+builtType+"|x64'"]), 'Command')
            CommandXml.set("Condition", "'$(Configuration)|$(Platform)'=='"+builtType+"|x64'")
            CommandXml.text = commands
            OutputsXml = getOrCreate(getOrCreate(ItemDefinitionGroupXML, "CustomBuildStep", "Command", ["Condition", "'$(Configuration)|$(Platform)'=='"+builtType+"|x64'"]), 'Outputs')
            OutputsXml.set("Condition", "'$(Configuration)|$(Platform)'=='"+builtType+"|x64'")
            OutputsXml.text = R'dummy_file.txt'

    ET.indent(tree, space="\t", level=0)
    tree.write(projectFile)


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

for vcxproj in tListVcxproj:
    isInList = False
    for proj_name, proj_file in listProjectNodeGLLib.items():
        if vcxproj.find(proj_name+'@') >= 0:
            isInList = True
            break
    if not isInList:
        updateProject(vcxproj, '')

updateProjectUser("nodegl", "")
updateProjectUser("ngl-python", "-b "+backend+R" $(ProjectDir)\..\..\builddir\tests\..\..\tests\compute.py compute_cubemap_load_store")
updateProjectUser("ngl-desktop", "-b "+backend)
updateProjectUser("ngl-serialize", R"$(ProjectDir)\..\..\builddir\tests\..\..\tests\compute.py compute_cubemap_load_store c:\tmp\compute_cubemap_load_store.ngl")


print("setup dev sln -> done")

subprocess.call([sys.executable, os.path.join(repoDir, "scripts", "launch_dev_sln.py")])