import os
import platform

path = "vendor/bin/premake/"
platformSystem = platform.system();
osCall = ""

if platformSystem == "Darwin":
	osCall = path + platformSystem + "/premake5 xcode4"

if platformSystem == "Windows":
	osCall = path + platformSystem + "/premake5.exe vs2017"

os.system(osCall)