import os
import platform
import sys
import getopt


thisFilePath = os.path.dirname(os.path.abspath(__file__))
cubemapgenEXEPath = os.path.realpath(thisFilePath + "\\..") + "\\tools\\cubemapgen\\bin\\Debug_windows_x86_64\\cubemapgen\\cubemapgen.exe"


def GenerateCubemapsFromEquirectangular(inputFile, outputDir, numSamples, writeMipChain, numMipLevels, mipOutputDir, quiet):
	
	quietVal = "false"
	if(quiet):
		quietVal = "true"

	writeMips = "false"
	if(writeMipChain):
		writeMips = "true"

	osCall = cubemapgenEXEPath + " -q=" + quietVal + " -i=" + inputFile + " -o=" + outputDir + " -n=" + str(numSamples) + " -m=" + writeMips + " -c=" + str(numMipLevels) + " -a=" + mipOutputDir
	os.system(osCall)

	return;


inputDir = ''


try:
	opts, args = getopt.getopt(sys.argv[1:], "i:")
except:
	print("Error running script")
	exit(1)

for opt, arg in opts:
	if opt in ['-i']:
		inputDir = arg


for packdir in os.listdir(inputDir):
	
	realpackdir = inputDir + packdir

	convolutionPackDir = inputDir + packdir + "_conv"

	if os.path.isdir(realpackdir):
		
		for subpackdir in os.listdir(realpackdir):
			
			realsubpackdir = realpackdir + '\\' + subpackdir

			if subpackdir == "hdr":
				
				for hdrfile in os.listdir(realsubpackdir):
					inputFile = realsubpackdir + '\\' + hdrfile
	
					GenerateCubemapsFromEquirectangular(inputFile, convolutionPackDir, 128, True, 1, realpackdir, False)
				break
			