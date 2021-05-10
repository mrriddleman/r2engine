import os
import platform
import subprocess 

generateCubemapsPath = "./r2engine/scripts/GenerateCubemaps.py"

subprocess.call(['python', generateCubemapsPath, '-i', 'D:\\Projects\\r2engine\\Sandbox\\assets\\Sandbox_Textures\\packs\\'])