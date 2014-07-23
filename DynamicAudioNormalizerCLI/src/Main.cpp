///////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer
// Copyright (C) 2014 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version, but always including the *additional*
// restrictions defined in the "License.txt" file.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
///////////////////////////////////////////////////////////////////////////////

//Internal
#include "Common.h"
#include "Parameters.h"
#include "AudioFileIO.h"

//MDynamicAudioNormalizer API
#include "DynamicAudioNormalizer.h"

//VLD
#ifdef _MSC_VER
#include <vld.h>
#endif

//C++
#include <ctime>
#include <algorithm>
#include <cmath>

//Linkage
#ifdef MDYNAMICAUDIONORMALIZER_STATIC
	static const CHR *LINKAGE = TXT("Static");
#else
	static const CHR *LINKAGE = TXT("Shared");
#endif

//Utility macros
#define BOOLIFY(X) ((X) ? TXT("on") : TXT("off"))
static const size_t FRAME_SIZE = 4096;

static const CHR *appName(const CHR* argv0)
{
	const CHR *appName  = argv0;
	while(const CHR *temp = STRCHR(appName, TXT('/' ))) appName = &temp[1];
	while(const CHR *temp = STRCHR(appName, TXT('\\'))) appName = &temp[1];
	return appName;
}

static const CHR *timeToString(CHR *timeBuffer, const size_t buffSize, const int64_t &length, const uint32_t &sampleRate)
{
	double durationInt;
	double durationFrc = modf(double(length) / double(sampleRate), &durationInt);

	const uint32_t msc = uint32_t(durationFrc * 1000.0);
	const uint32_t sec = uint32_t(durationInt) % 60;
	const uint32_t min = (uint32_t(durationInt) / 60) % 60;
	const uint32_t hrs = uint32_t(durationInt) / 3600;

	SNPRINTF(timeBuffer, buffSize, TXT("%u:%02u:%02u.%03u"), hrs, min, sec, msc);
	return timeBuffer;
}

static bool openFiles(const Parameters &parameters, AudioFileIO **sourceFile, AudioFileIO **outputFile)
{
	bool okay = true;

	PRINT(TXT("SourceFile: %s\n"), parameters.sourceFile());
	PRINT(TXT("OutputFile: %s\n"), parameters.outputFile());

	*sourceFile = new AudioFileIO();
	if(!(*sourceFile)->openRd(parameters.sourceFile()))
	{
		LOG2_WRN(TXT("Failed to open input file \"%s\" for reading!\n"), parameters.sourceFile());
		okay = false;
	}
	
	if(okay)
	{
		uint32_t channels, sampleRate, bitDepth;
		int64_t length;
		if((*sourceFile)->queryInfo(channels, sampleRate, length, bitDepth))
		{
			*outputFile = new AudioFileIO();
			if(!(*outputFile)->openWr(parameters.outputFile(), channels, sampleRate, bitDepth))
			{
				LOG2_WRN(TXT("Failed to open output file \"%s\" for writing!\n"), parameters.outputFile());
				okay = false;
			}

			CHR timeBuffer[32];
			PRINT(TXT("Properties: %u channels, %u Hz, %u bit/sample (Duration: %s)\n\n"), channels, sampleRate, bitDepth, timeToString(timeBuffer, 32, length, sampleRate));
		}
		else
		{
			LOG_WRN(TXT("Failed to determine source file properties!\n"));
			okay = false;
		}
	}

	if(!okay)
	{
		if(*sourceFile)
		{
			(*sourceFile)->close();
			MY_DELETE(*sourceFile);
		}
		if(*outputFile)
		{
			(*outputFile)->close();
			MY_DELETE(*outputFile);
		}
		return false;
	}

	return true;
}

static int processingLoop(MDynamicAudioNormalizer *normalizer, AudioFileIO *const sourceFile, AudioFileIO *const outputFile, double **buffer, const uint32_t channels, const int64_t length)
{
	static const CHR spinner[4] = { TXT('-'), TXT('\\'), TXT('|'), TXT('/') };
	static const CHR *progressStr = TXT("\rNormalization in progress: %5.1f%% [%c]");

	//Print progress
	PRINT(progressStr, 0.0, spinner[0]);
	FLUSH();

	//Rewind input
	if(!sourceFile->rewind())
	{
		PRINT(TXT("\n\n"));
		LOG_ERR(TXT("Failed to rewind the input file!"));
		return EXIT_FAILURE;
	}

	//Init status variables
	int64_t remaining = length;
	short indicator = 0, spinnerPos = 1;
	bool error = false;

	//Main audio processing loop
	while(remaining > 0)
	{
		if(++indicator > 256)
		{
			PRINT(progressStr, 99.9 * (double(length - remaining) / double(length)), spinner[spinnerPos++]);
			FLUSH(); indicator = 0; spinnerPos %= 4;
		}

		const int64_t readSize = std::min(remaining, int64_t(FRAME_SIZE));
		const int64_t samplesRead = sourceFile->read(buffer, readSize);

		remaining -= samplesRead;

		if(samplesRead != readSize)
		{
			error = true;
			break; /*read error must have ocurred*/
		}

		int64_t outputSize;
		normalizer->processInplace(buffer, samplesRead, outputSize);

		if(outputSize > 0)
		{
			if(outputFile->write(buffer, outputSize) != outputSize)
			{
				error = true;
				break; /*write error must have ocurred*/
			}
		}
	}

	//Flush all the delayed samples
	for(;;)
	{
		int64_t outputSize;
		normalizer->flushBuffer(buffer, int64_t(FRAME_SIZE), outputSize);

		if(outputSize > 0)
		{
			if(outputFile->write(buffer, outputSize) != outputSize)
			{
				error = true;
				break; /*write error must have ocurred*/
			}
		}
		else
		{
			break; /*failed to flush pending samples*/
		}
	}

	//Error checking
	if(!error)
	{
		PRINT(progressStr, 100.0, TXT('*'));
		FLUSH();
		PRINT(TXT("\nCompleted.\n\n"));
	}
	else
	{
		PRINT(TXT("\n\n"));
		LOG_ERR(TXT("I/O error encountered -> stopping!\n"));
	}

	return error ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int processFiles(const Parameters &parameters, AudioFileIO *const sourceFile, AudioFileIO *const outputFile, FILE *const logFile)
{
	int exitCode = EXIT_SUCCESS;

	//Get source info
	uint32_t channels, sampleRate, bitDepth;
	int64_t length;
	if(!sourceFile->queryInfo(channels, sampleRate, length, bitDepth))
	{
		LOG_WRN(TXT("Failed to determine source file properties!\n"));
		return EXIT_FAILURE;
	}

	//Create the normalizer
	MDynamicAudioNormalizer *normalizer = new MDynamicAudioNormalizer
	(
		channels,
		sampleRate,
		parameters.frameLenMsec(),
		parameters.channelsCoupled(),
		parameters.enableDCCorrection(),
		parameters.peakValue(),
		parameters.maxAmplification(),
		parameters.filterSize(),
		parameters.verboseMode(),
		logFile
	);
	
	//Initialze normalizer
	if(!normalizer->initialize())
	{
		LOG_ERR(TXT("Failed to initialize the normalizer instance!\n"));
		MY_DELETE(normalizer);
		return EXIT_FAILURE;
	}

	//Allocate buffers
	double **buffer = new double*[channels];
	for(uint32_t channel = 0; channel < channels; channel++)
	{
		buffer[channel] = new double[FRAME_SIZE];
	}
	
	//Run normalizer now!
	exitCode = processingLoop(normalizer, sourceFile, outputFile, buffer, channels, length);

	//Destroy the normalizer
	MY_DELETE(normalizer);

	//Memory clean-up
	for(uint32_t channel = 0; channel < channels; channel++)
	{
		MY_DELETE_ARRAY(buffer[channel]);
	}
	MY_DELETE_ARRAY(buffer);

	//Return result
	return exitCode;
}

static void printHelpScreen(int argc, CHR* argv[])
{
	Parameters defaults;

	PRINT(TXT("Usage:\n"));
	PRINT(TXT("  %s [options] -i <input,wav> -o <output,wav>\n"), appName(argv[0]));
	PRINT(TXT("\n"));
	PRINT(TXT("Options:\n"));
	PRINT(TXT("  -i --input <file>        Input audio file [required]\n"));
	PRINT(TXT("  -o --output <file>       Output audio file [required]\n"));
	PRINT(TXT("  -l --logfile <file>      Create log file\n"));
	PRINT(TXT("  -p --peak <value>        Target peak magnitude, 0.1-1 [default: %.2f]\n"), defaults.peakValue());
	PRINT(TXT("  -m --max-gain <value>    Maximum gain factor [default: %.2f]\n"), defaults.maxAmplification());
	PRINT(TXT("  -f --frame-len <value>   Frame length, in milliseconds [default: %u]\n"), defaults.frameLenMsec());
	PRINT(TXT("  -g --gauss-size <value>  Gauss filter size, in frames [default: %u]\n"), defaults.filterSize());
	PRINT(TXT("  -n --no-coupling         Disable channel coupling [default: %s]\n"), BOOLIFY(defaults.channelsCoupled()));
	PRINT(TXT("  -c --correct-dc          Enable the DC bias correction [default: %s]\n"), BOOLIFY(defaults.enableDCCorrection()));
	PRINT(TXT("  -v --verbose             Enable verbose console output\n"));
	PRINT(TXT("  -h --help                Print this help screen\n"));
	PRINT(TXT("\n"));
}

static void printLogo(void)
{
	uint32_t versionMajor, versionMinor, versionPatch;
	MDynamicAudioNormalizer::getVersionInfo(versionMajor, versionMinor, versionPatch);

	const char *buildDate, *buildTime, *buildCompiler, *buildArch; bool buildDebug;
	MDynamicAudioNormalizer::getBuildInfo(&buildDate, &buildTime, &buildCompiler, &buildArch, buildDebug);

	PRINT(TXT("---------------------------------------------------------------------------\n"));

	PRINT(TXT("Dynamic Audio Normalizer, Version %u.%02u-%u, %s\n"), versionMajor, versionMinor, versionPatch, LINKAGE);
	PRINT(TXT("Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.\n"));
	PRINT(TXT("Built on ") FMT_CHAR TXT(" at ") FMT_CHAR TXT(" with ") FMT_CHAR TXT(" for ") OS_TYPE TXT("-") FMT_CHAR TXT(".\n\n"), buildDate, buildTime, buildCompiler, buildArch);

	PRINT(TXT("This program is free software: you can redistribute it and/or modify\n"));
	PRINT(TXT("it under the terms of the GNU General Public License <http://www.gnu.org/>.\n"));
	PRINT(TXT("Note that this program is distributed with ABSOLUTELY NO WARRANTY.\n"));

	if(DYAUNO_DEBUG)
	{
		PRINT(TXT("!!! DEBUG BUILD !!! DEBUG BUILD !!! DEBUG BUILD !!! DEBUG BUILD !!!\n\n"));
	}

	PRINT(TXT("---------------------------------------------------------------------------\n\n"));

	if((DYAUNO_DEBUG) != buildDebug)
	{
		LOG_ERR(TXT("Trying to use DEBUG library with RELEASE binary or vice versa!\n"));
		exit(EXIT_FAILURE);
	}

	PRINT(TXT("Using ") FMT_CHAR TXT(", by Erik de Castro Lopo <erikd@mega-nerd.com>.\n\n"), AudioFileIO::libraryVersion());
}

int dynamicNormalizerMain(int argc, CHR* argv[])
{
	printLogo();

	Parameters parameters;
	if(!parameters.parseArgs(argc, argv))
	{
		LOG2_ERR(TXT("Invalid or incomplete command-line arguments have been detected.\n\nType \"%s --help\" for details!\n"), appName(argv[0]));
		return EXIT_FAILURE;
	}

	if(parameters.showHelp())
	{
		printHelpScreen(argc, argv);
		return EXIT_SUCCESS;
	}

	AudioFileIO *sourceFile = NULL, *outputFile = NULL;
	if(!openFiles(parameters, &sourceFile, &outputFile))
	{
		LOG_ERR(TXT("Failed to open input and/or output file!\n"));
		return EXIT_FAILURE;
	}

	FILE *logFile = NULL;
	if(parameters.dbgLogFile())
	{
		if(!(logFile = FOPEN(parameters.dbgLogFile(), TXT("w"))))
		{
			LOG2_WRN(TXT("Failed to open log file \"%s\""), parameters.dbgLogFile());
		}
	}

	const clock_t clockStart = clock();
	const int result = processFiles(parameters, sourceFile, outputFile, logFile);
	const double elapsed = double(clock() - clockStart) / double(CLOCKS_PER_SEC);

	sourceFile->close();
	outputFile->close();

	MY_DELETE(sourceFile);
	MY_DELETE(outputFile);

	PRINT(TXT("---------------------------------------------------------------------------\n\n"));
	PRINT(TXT("%s\n\nOperation took %.1f seconds.\n\n"), ((result != EXIT_SUCCESS) ? TXT("Sorry, something went wrong :-(") : TXT("Everything completed successfully :-)")), elapsed);

	return result;
}

int mainEx(int argc, CHR* argv[])
{
	int exitCode = EXIT_SUCCESS;

	try
	{
		exitCode = dynamicNormalizerMain(argc, argv);
	}
	catch(std::exception &e)
	{
		PRINT(TXT("\n\nGURU MEDITATION: Unhandeled C++ exception error: ") FMT_CHAR TXT("\n\n"), e.what());
		exitCode = EXIT_FAILURE;
	}
	catch(...)
	{
		PRINT(TXT("\n\nGURU MEDITATION: Unhandeled unknown C++ exception error!\n\n"));
		exitCode = EXIT_FAILURE;
	}

	return exitCode;
}

int MAIN(int argc, CHR* argv[])
{
	int exitCode = EXIT_SUCCESS;

	if(DYAUNO_DEBUG)
	{
		SYSTEM_INIT();
		exitCode = dynamicNormalizerMain(argc, argv);
	}
	else
	{
		TRY_SEH
		{
			SYSTEM_INIT();
			exitCode =  mainEx(argc, argv);
		}
		CATCH_SEH
		{
			PRINT(TXT("\n\nGURU MEDITATION: Unhandeled structured exception error!\n\n"));
			exitCode = EXIT_FAILURE;
		}
	}

	return exitCode;
}
