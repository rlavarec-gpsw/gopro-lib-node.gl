
import os
import shutil
import re
import xml.etree.ElementTree as ET

# go to root
repoDir = os.path.join(os.path.dirname(__file__), "..")
os.chdir(repoDir)

ET.register_namespace("", "http://schemas.microsoft.com/developer/msbuild/2003")

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
libnodegl_dev_sln = libnodegl_sln.replace(".sln", ".dev.sln")

nodegl_vcxproj = "nodegl@sha.vcxproj"
nodegl_dev_vcxproj = nodegl_vcxproj.replace(".vcxproj", ".dev.vcxproj")

shutil.copyfile(getFilename(libnodegl_sln), getFilename(libnodegl_dev_sln))
shutil.copyfile(getFilename(nodegl_vcxproj), getFilename(nodegl_dev_vcxproj))

listProjectToInclude = {"ngl-python": "..\\ngl-tools\\ngl-python@exe.vcxproj",
                        "ngl-desktop": "..\\ngl-tools\\ngl-desktop@exe.vcxproj",
                        "ngl-serialize": "..\\ngl-tools\\ngl-serialize@exe.vcxproj"}

listProjectNodeGLLib = {"nodegl": nodegl_dev_vcxproj}

with open(getFilename(libnodegl_dev_sln), 'r') as f:
    nodegl_vcxproj_data_file = "".join(f.readlines())
    projectID = re.search("Project\(\"{(.*)}\"\)", nodegl_vcxproj_data_file).group(1)

    nodegl_vcxproj_data_file = nodegl_vcxproj_data_file.replace(nodegl_vcxproj, nodegl_dev_vcxproj)

    for builtType in ["release", "debug"]:
        AllProjectEntry = ''
        AllProjectConfig = ''
        for name, project in listProjectToInclude.items():
            with open(getFilename(project), 'r') as f:
                project_data_file = "\n".join(f.readlines())
                project_uid = re.search("ProjectGuid>{(.*?)}", project_data_file).group(1)

                AllProjectEntry += 'Project("{'+projectID+'}") = "'+name+'", "'+project+'", "{'+project_uid+'}"' + "\n" + 'EndProject' + "\n"
                AllProjectConfig += '{'+project_uid+'}.'+builtType+'|x64.ActiveCfg = '+builtType+'|x64' + "\n" + '{'+project_uid+'}.'+builtType+'|x64.Build.0 = '+builtType+'|x64' + "\n"

        nodegl_vcxproj_data_file = nodegl_vcxproj_data_file.replace("EndProject\nGlobal\n", 'EndProject\n' + AllProjectEntry + 'Global' + "\n")
        nodegl_vcxproj_data_file = nodegl_vcxproj_data_file.replace("ActiveCfg = "+builtType+"|x64\n	EndGlobalSection\n", "ActiveCfg = "+builtType+"|x64\n	" + AllProjectConfig + "EndGlobalSection" + "\n")

with open(getFilename(libnodegl_dev_sln), 'w') as f:
    f.write(nodegl_vcxproj_data_file)


def getOrCreate(xmlObj, name):
    tXmlObj = xmlObj.find('{*}' + name)
    if tXmlObj == None:
        return ET.SubElement(xmlObj, name)
    return tXmlObj

def getVCXProjXml(filename):
    return ET.parse(filename)  # Must have a vcxproj

def updateProject(projectName, commands):

    if projectName in listProjectToInclude:
        projectUserFile = getFilename(listProjectToInclude[projectName])
    else:
        projectUserFile = getFilename(listProjectNodeGLLib[projectName])

    tree = getVCXProjXml(projectUserFile)
    ProjectXml = tree.getroot()

    for builtType in ["release", "debug"]:
        ItemDefinitionGroupXML = getOrCreate(ProjectXml, 'ItemDefinitionGroup')
        CommandXml = getOrCreate(getOrCreate(ItemDefinitionGroupXML, "CustomBuildStep"), 'Command')
        CommandXml.set("Condition", "'$(Configuration)|$(Platform)'=='"+builtType+"|x64'")
        CommandXml.text = commands
        OutputsXml = getOrCreate(getOrCreate(ItemDefinitionGroupXML, "CustomBuildStep"), 'Outputs')
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

    for builtType in ["release", "debug"]:
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
updateProjectUser("ngl-python", R"-b d3d12 $(ProjectDir)\..\..\builddir\tests\..\..\tests\compute.py compute_cubemap_load_store")
updateProjectUser("ngl-desktop", R"-b d3d12")
updateProjectUser("ngl-serialize", R"$(ProjectDir)\..\..\builddir\tests\..\..\tests\compute.py compute_cubemap_load_store c:\tmp\compute_cubemap_load_store.ngl")

print("setup dev sln -> done")