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

package com.muldersoft.dynaudnorm.test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;
import java.util.Formatter;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.Map;
import java.util.Queue;

import org.junit.After;
import org.junit.Before;
import org.junit.FixMethodOrder;
import org.junit.Test;
import org.junit.runners.MethodSorters;

import com.muldersoft.dynaudnorm.JDynamicAudioNormalizer;
import com.muldersoft.dynaudnorm.JDynamicAudioNormalizer.Logger;
import com.muldersoft.dynaudnorm.samples.AudioFileIO;

@FixMethodOrder(MethodSorters.NAME_ASCENDING)
public class JDynamicAudioNormalizerTest {

	// ------------------------------------------------------------------------------------------------
	// Randomization
	// ------------------------------------------------------------------------------------------------

	private static void double2DArrayRandom(double[][] output) {
		final SecureRandom rand = new SecureRandom();
		final double amp = Math.max(0.125, rand.nextDouble());
		for (final double[] channel : output) {
			for (int i = 0; i < channel.length; ++i) {
				final double sign = rand.nextBoolean() ? 1.0 : (-1.0);
				channel[i] = amp *sign * rand.nextDouble();
			}
		}
	}
	// ------------------------------------------------------------------------------------------------
	// Hash Computation
	// ------------------------------------------------------------------------------------------------

	public static String sha1sum(double[][] input) throws NoSuchAlgorithmException, IOException {
		final MessageDigest md = MessageDigest.getInstance("SHA-1");
		return byteArrayToHex(md.digest(double2DArrayToBytes(input)));
	}

	private static byte[] double2DArrayToBytes(double[][] input) throws IOException {
		try (ByteArrayOutputStream baos = new ByteArrayOutputStream()) {
			try (DataOutputStream dos = new DataOutputStream(baos)) {
				for (final double[] channel : input) {
					for (final double d : channel) {
						dos.writeDouble(d);
					}
				}
				dos.flush();
				return baos.toByteArray();
			}
		}
	}

	private static String byteArrayToHex(final byte[] hash) {
		try (final Formatter formatter = new Formatter()) {
			for (byte b : hash) {
				formatter.format("%02X", b);
			}
			return formatter.toString();
		}
	}

	// ------------------------------------------------------------------------------------------------
	// Test Functions
	// ------------------------------------------------------------------------------------------------

	@Before
	public void setUp() {
		System.out.println("\n============================= TEST BEGIN =============================");
	}

	@After
	public void shutDown() {
		System.out.println("============================= TEST DONE! =============================\n");
	}

	@Test
	public void test1_setLoggingHandler() {
		JDynamicAudioNormalizer.setLoggingHandler(new Logger() {
			public void log(final int level, final String message) {
				String label = "???";
				switch (level) {
				case 0:
					label = "DBG";
					break;
				case 1:
					label = "WRN";
					break;
				case 2:
					label = "ERR";
					break;
				}
				System.err.printf("[JDynAudNorm][%s] %s\n", label, message);
			}
		});
		System.out.println("Message handler installed successfully.");
	}

	@Test
	public void test2_getVersionInfo() {
		final int[] versionInfo = JDynamicAudioNormalizer.getVersionInfo();
		System.out.printf("Library Version: %d.%02d-%d\n", versionInfo[0], versionInfo[1], versionInfo[2]);
	}

	@Test
	public void test3_getBuildInfo() {
		final Map<String, String> buildInfo = JDynamicAudioNormalizer.getBuildInfo();
		for (final String key : buildInfo.keySet()) {
			System.out.println("Build Info: " + key + "=" + buildInfo.get(key));
		}
	}

	@Test
	public void test4_getConfiguration() {
		JDynamicAudioNormalizer instance = new JDynamicAudioNormalizer(2, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, true, false, false);
		final Map<String, Integer> buildInfo = instance.getConfiguration();
		for (final String key : buildInfo.keySet()) {
			System.out.printf("Configuration: %s=%s\n", key, buildInfo.get(key));
		}
		instance.close();
	}

	@Test
	public void test5_getInternalDelay() {
		JDynamicAudioNormalizer instance = new JDynamicAudioNormalizer(2, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, true, false, false);
		final long delayedSamples = instance.getInternalDelay();
		System.out.printf("Delayed Samples: %d\n", delayedSamples);
		instance.close();
	}

	@Test
	@SuppressWarnings("deprecation")
	public void test6_constructorAndRelease() {
		final int MAX_ROUNDS = 512;
		final int MAX_INSTANCES = 56;

		for (int i = 0; i < MAX_ROUNDS; i++) {
			System.out.println("--------------------------------------------------------------------------");
			System.out.printf("Round %d of %d\n", i + 1, MAX_ROUNDS);
			System.out.println("--------------------------------------------------------------------------");

			Queue<JDynamicAudioNormalizer> instances = new LinkedList<JDynamicAudioNormalizer>();

			// Constructor
			for (int k = 0; k < MAX_INSTANCES; k++) {
				JDynamicAudioNormalizer instance = new JDynamicAudioNormalizer(2, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, true, false, false);
				System.out.printf("Handle: %08X\n", instance.getHandle());
				instances.add(instance);
			}

			// Process at least one chunk of data with every instance
			double[][] sampleBuffer = new double[2][4096];
			for (Iterator<JDynamicAudioNormalizer> k = instances.iterator(); k.hasNext(); /* next */) {
				k.next().processInplace(sampleBuffer, sampleBuffer[0].length);
			}
			// Release
			while (!instances.isEmpty()) {
				instances.poll().close();
			}
		}
	}

	@Test
	public void test7_processRandomData() {
		final int MAX_PASS = 2;
		final int MAX_LENGTH = 9973;
		
		final double[] TARGET_RMS = { 0.0, 0.75 };
		final double[] COMPRESS = { 0.0, 5.0 };
		final boolean[] CCOUPLING = { true, false };
		final boolean[] DCCORRECT = { false, true };
		final boolean[] BOUNDARYM = { false, true };
		
		
		for(int p = 0; p < MAX_PASS; ++p) {
			double[][] sampleBuffer = new double[2][4096];
			try(final JDynamicAudioNormalizer instance = new JDynamicAudioNormalizer(2, 44100, 500, 31, 0.95, 10.0, TARGET_RMS[p], COMPRESS[p], CCOUPLING[p], DCCORRECT[p], BOUNDARYM[p])) {
				for(int i = 0; i < MAX_LENGTH; ++i) {
					if(i % 97 == 0) {
						System.out.printf("Pass %d of %d, Round %d of %d... [%.1f%%]\n", p+1, MAX_PASS, i, MAX_LENGTH, (((double)i) / MAX_LENGTH) * 100.0);
					}
					double2DArrayRandom(sampleBuffer);
					instance.processInplace(sampleBuffer, sampleBuffer[0].length);
				}
				System.out.printf("Pass %d of %d, Round %d of %d... [%.1f%%]\n", p+1, MAX_PASS, MAX_LENGTH, MAX_LENGTH, 100.0);
			}
		}
	}
	
	@Test
	public void test8_processAudioFile() throws NoSuchAlgorithmException, IOException {
		// Open input and output files
		System.out.println("Opening input and out files...");
		try(AudioFileIO.PCMFileReader reader = new AudioFileIO.PCMFileReader("Input.pcm", 2, AudioFileIO.PCMFileType.PCM_16BIT_LE)) {
			try (AudioFileIO.PCMFileWriter writer =  new AudioFileIO.PCMFileWriter("Output.pcm", 2, AudioFileIO.PCMFileType.PCM_16BIT_LE)) {
				double[][] sampleBuffer = new double[2][4096];
				
				System.out.println("Creating normalizer instance...");
				try(final JDynamicAudioNormalizer instance = new JDynamicAudioNormalizer(2, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, true, false, false)) {
					// Process samples
					System.out.println("Processing input samples...");
					for (;;) {
						int sampleCount = 0;
						try {
							sampleCount = reader.read(sampleBuffer);
						}
						catch (IOException e) {
							fail("Failed to read from input file!");
						}

						if (sampleCount > 0) {
							final long outputSamples = instance.processInplace(sampleBuffer, sampleCount);
							if (outputSamples > 0) {
								try {
									writer.write(sampleBuffer, (int) outputSamples);
								}
								catch (IOException e) {
									fail("Failed to write to output file!");
								}
							}
						}

						if (sampleCount < sampleBuffer[0].length) {
							break; /* EOF reached */
						}
					}

					// Flush pending samples
					System.out.println("Flushing remaining samples...");
					for (;;) {
						final long outputSamples = instance.flushBuffer(sampleBuffer);
						if (outputSamples > 0) {
							try {
								writer.write(sampleBuffer, (int) outputSamples);
							}
							catch (IOException e) {
								fail("Failed to write to output file!");
							}
						}
						else {
							break; /* No more samples */
						}
					}

					// Reset
					System.out.println("Resetting normalizer instance...");
					instance.reset(); /* This is NOT normally required, but we do it here to test the reset() API */
					
					// Close input and output
					System.out.println("Shutting down..."); /*Implicitly, due to AutoCloseable*/
				}
			}
		}
		
		// Finished
		System.out.println("Completed.");
	}

	@Test
	public void test9_processAudioFile_outOfPlace() throws NoSuchAlgorithmException, IOException {
		// Open input and output files
		System.out.println("Opening input and out files...");
		try(AudioFileIO.PCMFileReader reader = new AudioFileIO.PCMFileReader("Input.pcm", 2, AudioFileIO.PCMFileType.PCM_16BIT_LE)) {
			try (AudioFileIO.PCMFileWriter writer =  new AudioFileIO.PCMFileWriter("Output.pcm", 2, AudioFileIO.PCMFileType.PCM_16BIT_LE)) {
				double[][] sampleBufferSrc = new double[2][4096];
				double[][] sampleBufferDst = new double[2][4096];
				
				System.out.println("Creating normalizer instance...");
				try(final JDynamicAudioNormalizer instance = new JDynamicAudioNormalizer(2, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, true, false, false)) {
					// Process samples
					System.out.println("Processing input samples...");
					for (;;) {
						int sampleCount = 0;
						try {
							sampleCount = reader.read(sampleBufferSrc);
						}
						catch (IOException e) {
							fail("Failed to read from input file!");
						}

						final String hashOrig = sha1sum(sampleBufferSrc);
						if (sampleCount > 0) {
							final long outputSamples = instance.process(sampleBufferSrc, sampleBufferDst, sampleCount);
							if (outputSamples > 0) {
								try {
									writer.write(sampleBufferDst, (int) outputSamples);
								}
								catch (IOException e) {
									fail("Failed to write to output file!");
								}
							}
						}

						final String hashFinal = sha1sum(sampleBufferSrc);
						assertEquals("Input buffer contents have been distrubed!", hashOrig, hashFinal);

						if (sampleCount < sampleBufferSrc[0].length) {
							break; /* EOF reached */
						}
					}

					// Flush pending samples
					System.out.println("Flushing remaining samples...");
					for (;;) {
						final long outputSamples = instance.flushBuffer(sampleBufferDst);
						if (outputSamples > 0) {
							try {
								writer.write(sampleBufferDst, (int) outputSamples);
							}
							catch (IOException e) {
								fail("Failed to write to output file!");
							}
						}
						else {
							break; /* No more samples */
						}
					}

					// Reset
					System.out.println("Resetting normalizer instance...");
					instance.reset(); /* This is NOT normally required, but we do it here to test the reset() API */

					// Close input and output
					System.out.println("Shutting down..."); /*Implicitly, due to AutoCloseable*/
				}
			}
		}
		
		// Finished
		System.out.println("Completed.");
	}
}
