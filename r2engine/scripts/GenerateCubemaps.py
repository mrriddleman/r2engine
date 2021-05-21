import os
import platform
import sys
import getopt


thisFilePath = os.path.dirname(os.path.abspath(__file__))
cubemapgenEXEPath = os.path.realpath(thisFilePath + "\\..") + "\\tools\\cubemapgen\\bin\\Debug_windows_x86_64\\cubemapgen\\cubemapgen.exe"


def GenerateCubemapsFromEquirectangular(inputFile, outputDir, numSamples, prefilterRoughness, lutDFG, diffuseIrradiance, writeMipChain, numMipLevels, mipOutputDir, quiet):
	


	osCall = cubemapgenEXEPath + " -q=" + quiet + " -p=" + prefilterRoughness + " -l=" + lutDFG + " -d=" + diffuseIrradiance + " -i=" + inputFile + " -o=" + outputDir + " -n=" + str(numSamples) + " -m=" + writeMipChain + " -c=" + str(numMipLevels) + " -a=" + mipOutputDir
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

prefilter = "false"
lutDFG = "false"
diffuseIrradiance = "true"
writeMipChain = "true"
quiet = "false"
numMipLevels = 1
numSamples = 128

for packdir in os.listdir(inputDir):
	
	realpackdir = inputDir + packdir

	outputDir = inputDir + packdir

	if os.path.isdir(realpackdir):
		
		for subpackdir in os.listdir(realpackdir):
			
			realsubpackdir = realpackdir + '\\' + subpackdir

			if subpackdir == "hdr":
				
				for hdrfile in os.listdir(realsubpackdir):
					inputFile = realsubpackdir + '\\' + hdrfile
					GenerateCubemapsFromEquirectangular(inputFile, outputDir, numSamples, prefilter, lutDFG, diffuseIrradiance, writeMipChain, numMipLevels, realpackdir, quiet)
				break
			