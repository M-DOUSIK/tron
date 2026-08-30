import os

project_path = r"C:\Users\Dousik\Workspace\TRON\sessions\session_03_new\STM32CubeIDE\FSBL\.project"
cproject_path = r"C:\Users\Dousik\Workspace\TRON\sessions\session_03_new\STM32CubeIDE\FSBL\.cproject"

fw_repo = "C:/Users/Dousik/STM32Cube/Repository/STM32Cube_FW_N6_V1.4.0"
fw_repo_uri = "file:/" + fw_repo

parent2 = fw_repo + "/Projects/STM32N6570-DK/Applications/DCMIPP/DCMIPP_ContinuousMode"
parent2_uri = fw_repo_uri + "/Projects/STM32N6570-DK/Applications/DCMIPP/DCMIPP_ContinuousMode"

parent1 = parent2 + "/STM32CubeIDE"
parent1_uri = parent2_uri + "/STM32CubeIDE"

# Update .project
with open(project_path, 'r', encoding='utf-8') as f:
    p_content = f.read()

p_content = p_content.replace('PARENT-7-PROJECT_LOC', fw_repo_uri)
p_content = p_content.replace('PARENT-2-PROJECT_LOC', parent2_uri)
p_content = p_content.replace('PARENT-1-PROJECT_LOC', parent1_uri)

with open(project_path, 'w', encoding='utf-8') as f:
    f.write(p_content)

# Update .cproject
with open(cproject_path, 'r', encoding='utf-8') as f:
    c_content = f.read()

c_content = c_content.replace('../../../../../../../../', fw_repo + '/')
c_content = c_content.replace('../../../', parent2 + '/')

with open(cproject_path, 'w', encoding='utf-8') as f:
    f.write(c_content)

print("Absolute paths injected!")
