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

#include "Common.h"
#include "Parameters.h"
#include "AudioFileIO.h"
#include "DynamicNormalizer.h"
#include "Version.h"

#include <algorithm>

static const size_t FRAME_SIZE = 4096;

static const CHR *appName(const CHR* argv0)
{
	const CHR *appName  = argv0;
	while(const CHR *temp = STRCHR(appName, TXT('/' ))) appName = &temp[1];
	while(const CHR *temp = STRCHR(appName, TXT('\\'))) appName = &temp[1];
	return appName;
}

static bool openFiles(const Parameters &parameters, AudioFileIO **sourceFile, AudioFileIO **outputFile)
{
	bool okay = true;

	PRINT(TXT("Source file: %s\n"),   parameters.sourceFile());
	PRINT(TXT("Output file: %s\n\n"), parameters.outputFile());

	*sourceFile = new AudioFileIO();
	if(!(*sourceFile)->openRd(parameters.sourceFile()))
	{
		LOG_WRN(TXT("Failed to open input file for reading!\n"));
		okay = false;
	}
	
	if(okay)
	{
		uint32_t channels, sampleRate;
		int64_t length;
		if((*sourceFile)->queryInfo(channels, sampleRate, length))
		{
			*outputFile = new AudioFileIO();
			if(!(*outputFile)->openWr(parameters.outputFile(), channels, sampleRate))
			{
				LOG_WRN(TXT("Failed to open output file for writing!\n"));
				okay = false;
			}
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

static int runPass(DynamicNormalizer *normalizer, AudioFileIO *const sourceFile, AudioFileIO *const outputFile, double **buffer, const uint32_t channels, const int64_t length, const bool is2ndPass)
{
	static const CHR *progressStr = TXT("\rNormalization pass %d/2 in progress: %.1f%%");

	//Print progress
	PRINT(progressStr, (is2ndPass ? 2 : 1), 0.0);

	//Set encoding pass
	if(!normalizer->setPass(is2ndPass ? DynamicNormalizer::PASS_2ND : DynamicNormalizer::PASS_1ST))
	{
		PRINT(TXT("\n\n"));
		LOG_ERR(TXT("Failed to setup the pass!"));
		return EXIT_FAILURE;
	}

	//Rewind input
	if(!sourceFile->rewind())
	{
		PRINT(TXT("\n\n"));
		LOG_ERR(TXT("Failed to rewind the input file!"));
		return EXIT_FAILURE;
	}

	//Init status variables
	bool error = false;
	int64_t remaining = length;
	short indicator = 0;
	int64_t delayedSamples = 0;

	//Main audio processing loop
	while(remaining > 0)
	{
		if(++indicator > 512)
		{
			PRINT(progressStr, (is2ndPass ? 2 : 1), 99.9 * (double(length - remaining) / double(length)));
			indicator = 0;
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

		if(outputSize < samplesRead)
		{
			delayedSamples += (samplesRead - outputSize);
		}

		if(is2ndPass && (outputSize > 0))
		{
			if(outputFile->write(buffer, outputSize) != outputSize)
			{
				error = true;
				break; /*write error must have ocurred*/
			}
		}
	}

	//Flush all the delayed samples
	while(delayedSamples > 0)
	{
		for(uint32_t channel = 0; channel < channels; channel++)
		{
			memset(buffer[channel], 0, sizeof(double) * FRAME_SIZE);
		}

		int64_t outputSize;
		normalizer->processInplace(buffer, int64_t(FRAME_SIZE), outputSize);

		if(outputSize > 0)
		{
			const int64_t writeCount = std::min(outputSize, delayedSamples);
			if(is2ndPass && (outputFile->write(buffer, writeCount) != writeCount))
			{
				error = true;
				break; /*write error must have ocurred*/
			}
			delayedSamples -= writeCount;
		}
		else
		{
			break; /*failed to flush pending samples*/
		}
	}

	//Error checking
	if(!error)
	{
		PRINT(progressStr, (is2ndPass ? 2 : 1), 100.0);
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
	uint32_t channels, sampleRate;
	int64_t length;
	if(!sourceFile->queryInfo(channels, sampleRate, length))
	{
		LOG_WRN(TXT("Failed to determine source file properties!\n"));
		return EXIT_FAILURE;
	}

	//Allocate buffers
	double **buffer = new double*[channels];
	for(uint32_t channel = 0; channel < channels; channel++)
	{
		buffer[channel] = new double[FRAME_SIZE];
	}

	//Init normalizer
	DynamicNormalizer *normalizer = new DynamicNormalizer
	(
		channels,
		sampleRate,
		parameters.frameLenMsec(),
		parameters.channelsCoupled(),
		parameters.enableDCCorrection(),
		parameters.peakValue(),
		parameters.maxAmplification(),
		parameters.filterSize(),
		logFile
	);
	
	//Run 1st and 2nd pass
	for(int pass = 0; pass < 2; pass++)
	{
		exitCode = runPass(normalizer, sourceFile, outputFile, buffer, channels, length, (pass > 0));
		if(exitCode != EXIT_SUCCESS) break;
	}

	//Memory clean-up
	for(uint32_t channel = 0; channel < channels; channel++)
	{
		MY_DELETE_ARRAY(buffer[channel]);
	}
	MY_DELETE_ARRAY(buffer);
	MY_DELETE(normalizer);

	//Return result
	return exitCode;
}

int dynamicNormalizerMain(int argc, CHR* argv[])
{
	PRINT(TXT("\nDynamic Audio Normalizer, Version %u.%02u-%u\n"), DYAUNO_VERSION_MAJOR, DYAUNO_VERSION_MINOR, DYAUNO_VERSION_PATCH);
	PRINT(TXT("Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.\n"));
	PRINT(TXT("Built on %s at %s with %s for Win-%s.\n\n"), DYAUNO_BUILD_DATE, DYAUNO_BUILD_TIME, DYAUNO_COMPILER, DYAUNO_ARCH);

	PRINT(TXT("This program is free software: you can redistribute it and/or modify\n"));
	PRINT(TXT("it under the terms of the GNU General Public License <http://www.gnu.org/>.\n"));
	PRINT(TXT("Note that this program is distributed with ABSOLUTELY NO WARRANTY.\n\n"));

	Parameters parameters;
	if(!parameters.parseArgs(argc, argv))
	{
		LOG_ERR(TXT("Invalid or incomplete command-line arguments. Type \"%s --help\" for details!\n"), appName(argv[0]));
		return EXIT_FAILURE;
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
			LOG_WRN(TXT("Failed to open log file \"%s\""), parameters.dbgLogFile());
		}
	}

	const int result = processFiles(parameters, sourceFile, outputFile, logFile);

	sourceFile->close();
	outputFile->close();

	MY_DELETE(sourceFile);
	MY_DELETE(outputFile);

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
		__try
		{
			SYSTEM_INIT();
			exitCode =  mainEx(argc, argv);
		}
		__except(1)
		{
			PRINT(TXT("\n\nGURU MEDITATION: Unhandeled structured exception error!\n\n"));
			exitCode = EXIT_FAILURE;
		}
	}

	return exitCode;
}
