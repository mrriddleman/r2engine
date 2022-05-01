import os
import platform

thisFilePath = os.path.dirname(os.path.abspath(__file__))

assetconverter_exe_path = os.path.realpath(thisFilePath + "/../tools/assetconverter/bin/Debug_windows_x86_64/assetconverter/assetconverter.exe")

sandboxInputPath = "D:\\Projects\\r2engine\\Sandbox\\assets\\Sandbox_Textures\\packs"
sandboxOutputPath = "D:\\Projects\\r2engine\\Sandbox\\assets_bin\\Sandbox_Textures\\packs"

osCall = assetconverter_exe_path + " -i " + sandboxInputPath + " -o " + sandboxOutputPath

os.system(osCall)
