//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Java/JNI Wrapper
// Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// http://opensource.org/licenses/MIT
//////////////////////////////////////////////////////////////////////////////////

package com.muldersoft.dynaudnorm.samples;

import java.io.IOException;
import java.security.NoSuchAlgorithmException;
import java.util.Map;

import com.muldersoft.dynaudnorm.JDynamicAudioNormalizer;

public class DynamicAudioNormalizerExample {

	//Default arguments
	private static final int CHANNELS_DEFAULT = 2;
	private static final int SAMPLE_RATE_DEFAULT = 44100;
	private static final AudioFileIO.PCMFileType AUDIO_FORMAT_DEFAULT = AudioFileIO.PCMFileType.PCM_16BIT_LE;
	
	//Normalizer options
	private static final int FRAME_LEN = 500;	//Frame length in MSec
	private static final int FILTER_SIZE = 31;	//Filter size in frames
	
	//------------------------------------------------------------------------------------------------
	// Print Logo
	//------------------------------------------------------------------------------------------------
	
	private static void printLogo() {
		int [] version = null;
		Map<String,String> buildInfo = null;
		try	{
			try	{
				version = JDynamicAudioNormalizer.getVersionInfo();
				buildInfo = JDynamicAudioNormalizer.getBuildInfo();
			}
			catch(Exception ex) {
				throw new Error("Failed to initialize DynamicAudioNormalizer library!", ex);
			}
		}
		finally {
			System.out.println("\n===========================================================================");
			if((version != null) && (version.length >= 3)) {
				System.out.printf("Dynamic Audio Normalizer, Version %d.%02d-%d\n", version[0], version[1], version[2]);
			}
			else {
				System.out.println("Dynamic Audio Normalizer, Unknown Version");
			}
			if((buildInfo != null) && buildInfo.containsKey("BuildDate")) {
				System.out.printf("Copyright (c) 2014-%s LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.\n", buildInfo.get("BuildDate").substring(7));
			}
			else {
				System.out.println("Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.");
			}
			System.out.println();
			System.out.println("This program is free software: you can redistribute it and/or modify");
			System.out.println("it under the terms of the GNU General Public License <http://www.gnu.org/>.");
			System.out.println("Note that this program is distributed with ABSOLUTELY NO WARRANTY.");
			System.out.println("===========================================================================\n");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Processing Loop
	//------------------------------------------------------------------------------------------------

	public static void processFile(final String inputFile, final String outputFile, final int channles, final int sampleRate, final AudioFileIO.PCMFileType audioFormat) throws NoSuchAlgorithmException, IOException {
		// Open input and output files
		System.out.print("Opening input and out files...");
		try(AudioFileIO.PCMFileReader reader = new AudioFileIO.PCMFileReader(inputFile, channles, audioFormat)) {
			try (AudioFileIO.PCMFileWriter writer =  new AudioFileIO.PCMFileWriter(outputFile, channles, audioFormat)) {
				double[][] sampleBuffer = new double[channles][4096];
				
				System.out.print(" Done!\nCreating normalizer instance...");
				try(final JDynamicAudioNormalizer instance = new JDynamicAudioNormalizer(channles, sampleRate, FRAME_LEN, FILTER_SIZE, 0.95, 10.0, 0.0, 0.0, true, false, false)) {
					// Process samples
					System.out.print(" Done!\nProcessing input samples...");
					int progressUpdate = 0;
					for (;;) {
						int sampleCount = 0;
						try {
							sampleCount = reader.read(sampleBuffer);
						}
						catch (IOException e) {
							throw new IOException("Failed to read data from input file!", e);
						}
						
						if (sampleCount > 0) {
							final long outputSamples = instance.processInplace(sampleBuffer, sampleCount);
							if (outputSamples > 0) {
								try {
									writer.write(sampleBuffer, (int) outputSamples);
								}
								catch (IOException e) {
									throw new IOException("Failed to write data to output file!", e);
								}
							}
							if(++progressUpdate >= 13) {
								progressUpdate = 0;
								System.out.print(".");
							}
						}
						
						if (sampleCount < sampleBuffer[0].length) {
							break; /* EOF reached */
						}
					}
					
					// Flush pending samples
					System.out.print(" Done!\nFlushing remaining samples...");
					for (;;) {
						final long outputSamples = instance.flushBuffer(sampleBuffer);
						if (outputSamples > 0) {
							try {
								writer.write(sampleBuffer, (int) outputSamples);
							}
							catch (IOException e) {
								throw new IOException("Failed to write data to output file!", e);
							}
							if(++progressUpdate >= 13) {
								progressUpdate = 0;
								System.out.print(".");
							}
							
						}
						else {
							break; /* No more samples */
						}
					}
					
					// Reset
					System.out.print(" Done!\nResetting normalizer instance...");
					instance.reset(); /* This is NOT normally required, but we do it here to test the reset() API */
					
					// Close input and output
					System.out.print(" Done!\nShutting down..."); /*Implicitly, due to AutoCloseable*/
				}
			}
		}
		
		// Finished
		System.out.println(" Done!\n\nAll done. Goodbye.");
	}
	
	//------------------------------------------------------------------------------------------------
	// Main
	//------------------------------------------------------------------------------------------------

	public static void main(final String [] args)
	{
		try {
			//Print program logo
			printLogo();
			
			//Check number of arguments
			if(args.length < 2) {
				System.out.println("Usage:");
				System.out.println("  java -jar Example.jar [<channel_count> [<sample_rate> [<sample_format>]]] <input.pcm> <output.pcm>");
				System.exit(-1);
			}
			
			//Initialize arguments to default values
			int channels = CHANNELS_DEFAULT;
			int sampleRate = SAMPLE_RATE_DEFAULT;
			AudioFileIO.PCMFileType audioFormat = AUDIO_FORMAT_DEFAULT;
			
			//Parse optional arguments from command-line
			try {
				if(args.length > 2) {
					channels = Integer.parseInt(args[0]);
					if(args.length > 3) {
						sampleRate = Integer.parseInt(args[1]);
						if(args.length > 4) {
							audioFormat = AudioFileIO.PCMFileType.valueOf(args[2]);
						}
					}
				}
			}
			catch(Exception e) {
				throw new IllegalArgumentException("Invalid command-line argument given!", e);
			}
			
			//Process the audio file
			final int offset = Math.min(args.length, 5) - 2;
			processFile(args[offset], args[offset + 1], channels, sampleRate, audioFormat);
		}
		catch(Throwable e) {
			System.err.println("\n\nERROR: " + e.getMessage() + "\n");
			e.printStackTrace(System.err);
		}
	}

}
