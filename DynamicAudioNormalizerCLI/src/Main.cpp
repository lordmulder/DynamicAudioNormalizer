//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Audio Processing Utility
// Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// http://www.gnu.org/licenses/gpl-2.0.txt
//////////////////////////////////////////////////////////////////////////////////

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

//Internal
#include "Common.h"
#include "Platform.h"
#include "Parameters.h"
#include "AudioIO.h"

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
#include <inttypes.h>

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
	if(length != INT64_MAX)
	{
		double durationInt;
		double durationFrc = modf(double(length) / double(sampleRate), &durationInt);

		const uint32_t msc = uint32_t(durationFrc * 1000.0);
		const uint32_t sec = uint32_t(durationInt) % 60;
		const uint32_t min = (uint32_t(durationInt) / 60) % 60;
		const uint32_t hrs = uint32_t(durationInt) / 3600;

		SNPRINTF(timeBuffer, buffSize, TXT("%u:%02u:%02u.%03u"), hrs, min, sec, msc);
	}
	else
	{
		SNPRINTF(timeBuffer, buffSize, TXT("Unknown"));
	}

	return timeBuffer;
}

static void loggingCallback_default(const int logLevel, const char *const message)
{
	switch(logLevel)
	{
	case MDynamicAudioNormalizer::LOG_LEVEL_WRN:
		PRINT2_WRN(FMT_chr, message);
		break;
	case MDynamicAudioNormalizer::LOG_LEVEL_ERR:
		PRINT2_ERR(FMT_chr, message);
		break;
	}
}

static void loggingCallback_verbose(const int logLevel, const char *const message)
{
	switch(logLevel)
	{
	case MDynamicAudioNormalizer::LOG_LEVEL_NFO:
		PRINT2_NFO(FMT_chr, message);
		break;
	case MDynamicAudioNormalizer::LOG_LEVEL_WRN:
		PRINT2_WRN(FMT_chr, message);
		break;
	case MDynamicAudioNormalizer::LOG_LEVEL_ERR:
		PRINT2_ERR(FMT_chr, message);
		break;
	}
}

static bool openFiles(const Parameters &parameters, AudioIO **sourceFile, AudioIO **outputFile)
{
	uint32_t channels = 0, sampleRate = 0, bitDepth = 0;
	int64_t length = 0;

	MY_DELETE(*sourceFile);
	MY_DELETE(*outputFile);

	//Auto-detect source file type
	const CHR *sourceLibrary = parameters.sourceLibrary();
	if (!sourceLibrary)
	{
		sourceLibrary = AudioIO::detectSourceType(parameters.sourceFile());
	}

	//Print Audio I/O information
	PRINT(TXT("Using ") FMT_CHR TXT("\n\n"), AudioIO::getLibraryVersion(sourceLibrary));
	PRINT(TXT("SourceFile: ") FMT_CHR TXT("\n"),   STRCASECMP(parameters.sourceFile(), TXT("-")) ? parameters.sourceFile() : TXT("<STDIN>" ));
	PRINT(TXT("OutputFile: ") FMT_CHR TXT("\n\n"), STRCASECMP(parameters.outputFile(), TXT("-")) ? parameters.outputFile() : TXT("<STDOUT>"));

	//Open *source* file
	*sourceFile = AudioIO::createInstance(sourceLibrary);
	if(!(*sourceFile)->openRd(parameters.sourceFile(), parameters.inputChannels(), parameters.inputSampleRate(), parameters.inputBitDepth()))
	{
		PRINT2_WRN(TXT("Failed to open input file \"") FMT_CHR TXT("\" for reading!\n"), parameters.sourceFile());
		MY_DELETE(*sourceFile);
		return false;
	}
	
	//Query file properties
	if((*sourceFile)->queryInfo(channels, sampleRate, length, bitDepth))
	{
		CHR timeBuffer[32];
		PRINT(TXT("Properties: %u channels, %u Hz, %u bit/sample (Duration: ") FMT_CHR TXT(")\n\n"), channels, sampleRate, bitDepth, timeToString(timeBuffer, 32, length, sampleRate));
	}
	else
	{
		PRINT_WRN(TXT("Failed to determine source file properties!\n"));
		MY_DELETE(*sourceFile);
		return false;
	}

	//Open *output* file
	*outputFile = AudioIO::createInstance();
	if(!(*outputFile)->openWr(parameters.outputFile(), channels, sampleRate, bitDepth, parameters.outputFormat()))
	{
		PRINT2_WRN(TXT("Failed to open output file \"") FMT_CHR TXT("\" for writing!\n"), parameters.outputFile());
		MY_DELETE(*sourceFile);
		MY_DELETE(*outputFile);
		return false;
	}

	if(parameters.verboseMode())
	{
		CHR info[128];
		(*sourceFile)->getFormatInfo(info, 128);
		PRINT(TXT("SourceInfo: ") FMT_CHR TXT("\n"), info);
		(*outputFile)->getFormatInfo(info, 128);
		PRINT(TXT("OutputInfo: ") FMT_CHR TXT("\n\n"), info);
	}

	return true;
}

static void printProgress(const int64_t &length, const int64_t &remaining, short &spinnerPos, bool done = false)
{
	static const CHR spinner[4] = { TXT('-'), TXT('\\'), TXT('|'), TXT('/') };
	
	if(length != INT64_MAX)
	{
		const double progress = 99.99 * (double(length - remaining) / double(length));
		PRINT(TXT("\rNormalization in progress: %5.1f%% [%c]"), done ? std::max(progress, 100.0) : progress, done ? TXT('*') : spinner[spinnerPos++]);
	}
	else
	{
		PRINT(TXT("\rNormalization in progress... [%c]"), done ? TXT('*') : spinner[spinnerPos++]);
	}

	spinnerPos %= 4;
	FLUSH();
}

static int processingLoop(MDynamicAudioNormalizer *normalizer, AudioIO *const sourceFile, AudioIO *const outputFile, double **buffer, const uint32_t channels, const int64_t length)
{
	//Init status variables
	int64_t remaining = length;
	short indicator = 0, spinnerPos = 1;
	bool error = false;

	//Print progress
	printProgress(length, remaining, spinnerPos);

	//Main audio processing loop
	for(;;)
	{
		if((++indicator >= 256))
		{
			printProgress(length, remaining, spinnerPos);
			indicator = 0; 
		}

		const int64_t samplesRead = sourceFile->read(buffer, int64_t(FRAME_SIZE));
		if(samplesRead > 0)
		{
			if(length != INT64_MAX)
			{
				remaining -= samplesRead;
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

		if(samplesRead < int64_t(FRAME_SIZE))
		{
			break; /*end of file*/
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
			break; /*no more pending samples*/
		}
	}


	//Completed
	printProgress(length, remaining, spinnerPos, (!error));
	PRINT(TXT("\n") FMT_CHR TXT(".\n\n"), error ? TXT("Finished") : TXT("Aborted"));

	//Error checking
	if(error)
	{
		PRINT_ERR(TXT("I/O error encountered -> stopping!\n"));
	}
	else
	{
		const int64_t samples_delta = (length != INT64_MAX) ? abs(remaining) : (-1);
		if (samples_delta > 0)
		{
			const double delta_fract = double(samples_delta) / double(length);
			error = (delta_fract >= 0.25); /*read error*/
			PRINT2_WRN(TXT("Audio reader got %g (%.1f%%) ") FMT_CHR TXT(" samples than projected!\n"), double(samples_delta), delta_fract, (remaining > 0) ? TXT("less") : TXT("more"));
		}
	}

	return error ? EXIT_FAILURE : EXIT_SUCCESS;
}

static int processFiles(const Parameters &parameters, AudioIO *const sourceFile, AudioIO *const outputFile, FILE *const logFile)
{
	int exitCode = EXIT_SUCCESS;

	//Get source info
	uint32_t channels, sampleRate, bitDepth;
	int64_t length;
	if(!sourceFile->queryInfo(channels, sampleRate, length, bitDepth))
	{
		PRINT_WRN(TXT("Failed to determine source file properties!\n"));
		return EXIT_FAILURE;
	}

	//Create the normalizer instance
	MDynamicAudioNormalizer *normalizer = new MDynamicAudioNormalizer
	(
		channels,
		sampleRate,
		parameters.frameLenMsec(),
		parameters.filterSize(),
		parameters.peakValue(),
		parameters.maxAmplification(),
		parameters.targetRms(),
		parameters.compressFactor(),
		parameters.channelsCoupled(),
		parameters.enableDCCorrection(),
		parameters.altBoundaryMode(),
		logFile
	);
	
	//Initialze normalizer
	if(!normalizer->initialize())
	{
		PRINT_ERR(TXT("Failed to initialize the normalizer instance!\n"));
		MY_DELETE(normalizer);
		return EXIT_FAILURE;
	}

	//Allocate buffers
	double **buffer = new double*[channels];
	for(uint32_t channel = 0; channel < channels; channel++)
	{
		buffer[channel] = new double[FRAME_SIZE];
	}
	
	//Some extra spacing in VERBOSE mode
	if(parameters.verboseMode())
	{
		PRINT(TXT("\n"));
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

	//Some extra spacing in VERBOSE mode
	if(parameters.verboseMode())
	{
		PRINT(TXT("\n"));
	}

	//Return result
	return exitCode;
}

static void printHelpScreen(int argc, CHR* argv[])
{
	Parameters defaults;

	PRINT(TXT("Basic Usage:\n"));
	PRINT(TXT("  ") FMT_CHR TXT(" [options] -i <input.wav> -o <output.wav>\n"), appName(argv[0]));
	PRINT(TXT("\n"));
	PRINT(TXT("Input/Output:\n"));
	PRINT(TXT("  -i --input <file>        Input audio file [required]\n"));
	PRINT(TXT("  -d --input-lib <value>   Input decoder library [default: auto-detect]\n"));
	PRINT(TXT("  -o --output <file>       Output audio file [required]\n"));
	PRINT(TXT("  -t --output-fmt <value>  Output format [default: auto-detect]\n"));
	PRINT(TXT("\n"));
	PRINT(TXT("Algorithm Tweaks:\n"));
	PRINT(TXT("  -f --frame-len <value>   Frame length, in milliseconds [default: %u]\n"),               defaults.frameLenMsec());
	PRINT(TXT("  -g --gauss-size <value>  Gauss filter size, in frames [default: %u]\n"),                defaults.filterSize());
	PRINT(TXT("  -p --peak <value>        Target peak magnitude, 0.1-1 [default: %.2f]\n"),              defaults.peakValue());
	PRINT(TXT("  -m --max-gain <value>    Maximum gain factor [default: %.2f]\n"),                       defaults.maxAmplification());
	PRINT(TXT("  -r --target-rms <value>  Target RMS value [default: %.2f]\n"),                          defaults.targetRms());
	PRINT(TXT("  -n --no-coupling         Disable channel coupling [default: ") FMT_CHR TXT("]\n"),      BOOLIFY(defaults.channelsCoupled()));
	PRINT(TXT("  -c --correct-dc          Enable the DC bias correction [default: ") FMT_CHR TXT("]\n"), BOOLIFY(defaults.enableDCCorrection()));
	PRINT(TXT("  -b --alt-boundary        Use alternative boundary mode [default: ") FMT_CHR TXT("]\n"), BOOLIFY(defaults.altBoundaryMode()));
	PRINT(TXT("  -s --compress <value>    Compress the input data [default: %.2f]\n"),                   defaults.compressFactor());
	PRINT(TXT("\n"));
	PRINT(TXT("Diagnostics:\n"));
	PRINT(TXT("  -v --verbose             Output additional diagnostic info\n"));
	PRINT(TXT("  -l --log-file <file>     Create a log file\n"));
	PRINT(TXT("  -h --help                Print *this* help screen\n"));
	PRINT(TXT("\n"));
	PRINT(TXT("Raw Data Options:\n"));
	PRINT(TXT("  --input-bits <value>     Bits per sample, e.g. '16' or '32'\n"));
	PRINT(TXT("  --input-chan <value>     Number of channels, e.g. '2' for Stereo\n"));
	PRINT(TXT("  --input-rate <value>     Sample rate in Hertz, e.g. '44100'\n"));
	PRINT(TXT("\n"));
}

static void printLogo(void)
{
	uint32_t versionMajor, versionMinor, versionPatch;
	MDynamicAudioNormalizer::getVersionInfo(versionMajor, versionMinor, versionPatch);

	const char *buildDate, *buildTime, *buildCompiler, *buildArch; bool buildDebug;
	MDynamicAudioNormalizer::getBuildInfo(&buildDate, &buildTime, &buildCompiler, &buildArch, buildDebug);

	PRINT(TXT("---------------------------------------------------------------------------\n"));

	PRINT(TXT("Dynamic Audio Normalizer, Version %u.%02u-%u, ") FMT_CHR TXT("\n"), versionMajor, versionMinor, versionPatch, LINKAGE);
	PRINT(TXT("Copyright (c) 2014-") FMT_chr TXT(" LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.\n"), &buildDate[7]);
	PRINT(TXT("Built on ") FMT_chr TXT(" at ") FMT_chr TXT(" with ") FMT_chr TXT(" for ") OS_TYPE TXT("-") FMT_chr TXT(".\n\n"), buildDate, buildTime, buildCompiler, buildArch);

	PRINT(TXT("This program is free software: you can redistribute it and/or modify\n"));
	PRINT(TXT("it under the terms of the GNU General Public License <http://www.gnu.org/>.\n"));
	PRINT(TXT("Note that this program is distributed with ABSOLUTELY NO WARRANTY.\n"));

	if(DYNAUDNORM_DEBUG)
	{
		PRINT(TXT("\n!!! DEBUG BUILD !!! DEBUG BUILD !!! DEBUG BUILD !!! DEBUG BUILD !!!\n"));
	}

	PRINT(TXT("---------------------------------------------------------------------------\n\n"));

	if((DYNAUDNORM_DEBUG) != buildDebug)
	{
		PRINT_ERR(TXT("Trying to use DEBUG library with RELEASE binary or vice versa!\n"));
		exit(EXIT_FAILURE);
	}
}

int dynamicNormalizerMain(int argc, CHR* argv[])
{
	printLogo();

	Parameters parameters;
	if(!parameters.parseArgs(argc, argv))
	{
		PRINT2_ERR(TXT("Invalid or incomplete command-line arguments have been detected.\n\nPlease type \"") FMT_CHR TXT(" --help\" for details!\n"), appName(argv[0]));
		return EXIT_FAILURE;
	}

	if(parameters.showHelp())
	{
		printHelpScreen(argc, argv);
		return EXIT_SUCCESS;
	}

	MDynamicAudioNormalizer::setLogFunction(parameters.verboseMode() ? loggingCallback_verbose : loggingCallback_default);

	AudioIO *sourceFile = NULL, *outputFile = NULL;
	if(!openFiles(parameters, &sourceFile, &outputFile))
	{
		PRINT_ERR(TXT("Failed to open input and/or output file -> aborting!\n"));
		return EXIT_FAILURE;
	}

	FILE *logFile = NULL;
	if(parameters.dbgLogFile())
	{
		if(!(logFile = FOPEN(parameters.dbgLogFile(), TXT("w"))))
		{
			PRINT_WRN(TXT("Failed to open log file -> No log file will be created!\n"));
		}
	}

	const clock_t clockStart = clock();
	const int result = processFiles(parameters, sourceFile, outputFile, logFile);
	const double elapsed = double(clock() - clockStart) / double(CLOCKS_PER_SEC);

	sourceFile->close();
	outputFile->close();

	MY_DELETE(sourceFile);
	MY_DELETE(outputFile);

	if(logFile)
	{
		fclose(logFile);
		logFile = NULL;
	}

	PRINT(TXT("--------------\n\n"));
	PRINT(FMT_CHR TXT("\n\nOperation took %.1f seconds.\n\n"), ((result != EXIT_SUCCESS) ? TXT("Sorry, something went wrong :-(") : TXT("Everything completed successfully :-)")), elapsed);

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
		PRINT(TXT("\n\nGURU MEDITATION: Unhandeled C++ exception error: ") FMT_chr TXT("\n\n"), e.what());
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

	if(DYNAUDNORM_DEBUG)
	{
		SYSTEM_INIT(true);
		exitCode = dynamicNormalizerMain(argc, argv);
	}
	else
	{
		TRY_SEH
		{
			SYSTEM_INIT(false);
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
