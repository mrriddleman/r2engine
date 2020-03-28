import os
import platform
import subprocess 

path = os.path.dirname(os.path.abspath(__file__)) + "/vendor/bin/premake/"
generateCodePath = "./r2engine/scripts/GenerateFlatbufferCode.py"

subprocess.call(['python', generateCodePath])

platformSystem = platform.system();
osCall = ""

if platformSystem == "Darwin":
	osCall = path + platformSystem + "/premake5 xcode4"

if platformSystem == "Windows":
	osCall = path + platformSystem + "/premake5.exe vs2019"

os.system(osCall)