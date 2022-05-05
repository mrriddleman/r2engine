import os
import platform

thisFilePath = os.path.dirname(os.path.abspath(__file__))

assetconverter_exe_path = os.path.realpath(thisFilePath + "/../tools/assetconverter/bin/Publish_windows_x86_64/assetconverter/assetconverter.exe")

sandboxInputPath = "D:\\Projects\\r2engine\\Sandbox\\assets\\Sandbox_Textures\\packs"
sandboxOutputPath = "D:\\Projects\\r2engine\\Sandbox\\assets_bin\\Sandbox_Textures\\packs"

osCall = assetconverter_exe_path + " -i " + sandboxInputPath + " -o " + sandboxOutputPath

os.system(osCall)

r2engineInputPath = "D:\\Projects\\r2engine\\r2engine\\assets\\textures\\packs"
r2engineOutputPath = "D:\\Projects\\r2engine\\r2engine\\assets_bin\\textures\\packs"

osCall = assetconverter_exe_path + " -i " + r2engineInputPath + " -o " + r2engineOutputPath

os.system(osCall)