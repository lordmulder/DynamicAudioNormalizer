//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Java/JNI Wrapper
// Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
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

import static org.junit.Assert.fail;

import java.io.File;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.LinkedList;
import java.util.Map;
import java.util.Queue;

import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.UnsupportedAudioFileException;

import org.junit.After;
import org.junit.Before;
import org.junit.FixMethodOrder;
import org.junit.Test;
import org.junit.runners.MethodSorters;

import com.muldersoft.dynaudnorm.JDynamicAudioNormalizer;
import com.muldersoft.dynaudnorm.JDynamicAudioNormalizer.Logger;
import com.sun.media.sound.WaveFileReader;

@FixMethodOrder(MethodSorters.NAME_ASCENDING)
public class DynamicAudioNormalizerTest
{
	//------------------------------------------------------------------------------------------------
	// Wave Reader
	//------------------------------------------------------------------------------------------------

	static class Reader
	{
		private final WaveFileReader audioReader;
		private final AudioInputStream audioInputStream;

		final int channels;
		final int sampleRate;
		
		private byte [] tempBuffer = null; 
		
		public Reader(final String fileName) throws UnsupportedAudioFileException, IOException
		{
			audioReader = new WaveFileReader();
			audioInputStream = audioReader.getAudioInputStream(new File(fileName));
			
			AudioFormat inputFormat = audioInputStream.getFormat();
			if(inputFormat.getSampleSizeInBits() != 16)
			{
				throw new IOException("Unssuported format. Only 16-Bit is supported!");
			}

			channels = inputFormat.getChannels();
			sampleRate = Math.round(inputFormat.getSampleRate());
		}
		
		public int read(double [][] buffer) throws IOException
		{
			if(buffer.length < channels)
			{
				throw new IOException("Output array dimension is too small for channel count!");
			}
			
			final int requiredBuffSize =  buffer[0].length * channels * 2;
			if((tempBuffer == null) || (tempBuffer.length < requiredBuffSize))
			{
				tempBuffer = new byte[requiredBuffSize];
			}

			final int readSize = audioInputStream.read(tempBuffer, 0, requiredBuffSize);
			if(readSize > 2 * channels)
			{
				final int sampleCount = readSize / (2 * channels);
				ByteBuffer byteWrapper = ByteBuffer.wrap(tempBuffer);
				for(int i = 0; i < sampleCount; i++)
				{
					for(int c = 0; c < channels; c++)
					{
						buffer[c][i] = ((double)byteWrapper.getShort()) / ((double)Short.MAX_VALUE);
					}
				}
				return sampleCount;
			}
			
			return 0;
		}
		
		public int getChannels()
		{
			return channels;
		}

		public int sampleRate()
		{
			return sampleRate;
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Test Functions
	//------------------------------------------------------------------------------------------------

	@Before
	public void setUp()
	{
		System.out.println("============================= TEST BEGIN =============================");
	}
	
	@After
	public void shutDown()
	{
		System.out.println("============================= TEST DONE! =============================");
		System.out.println();
	}

	@Test
	public void test1_SetLoggingHandler()
	{
		JDynamicAudioNormalizer.setLoggingHandler(new Logger()
		{
			public void log(int level, String message) {
				System.err.println("[JDynAudNorm] " + message);
			}
		});
		System.out.println("Message handler installed successfully.");
	}

	@Test
	public void test2_GetVersionInfo()
	{
		final int[] versionInfo = JDynamicAudioNormalizer.getVersionInfo();
		System.out.println("Library Version: " + versionInfo[0] + "." + versionInfo[1] + "-" + versionInfo[2]);
	}
	
	@Test
	public void test3_GetBuildInfo()
	{
		final Map<String,String> buildInfo = JDynamicAudioNormalizer.getBuildInfo();
		for(final String key : buildInfo.keySet())
		{
			System.out.println("Build Info: " + key + "=" + buildInfo.get(key));
		}
	}
	
	@Test
	public void test4_ConstructorAndRelease()
	{
		for(int i = 0; i < 8; i++)
		{
			Queue<JDynamicAudioNormalizer> instances = new LinkedList<JDynamicAudioNormalizer>();

			// Constructor
			for(int k = 0; k < 32; k++)
			{
				JDynamicAudioNormalizer instance = new JDynamicAudioNormalizer(2, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, true, false, false);
				instances.add(instance);
			}
			
			// Release
			while(!instances.isEmpty())
			{
				instances.poll().release();
			}
		}
	}
	
	@Test
	public void test5_ProcessWaveFile()
	{
		Reader reader = null;
		try
		{
			reader = new Reader("E:\\Images\\WhaleOnThis.wav");
		}
		catch (Exception e)
		{
			fail("Failed to open ionput audio file!");
		}
		
		double [][] sampleBuffer = new double[reader.getChannels()][4096];
		for(;;)
		{
			try
			{
				final int sampleCount = reader.read(sampleBuffer);
				if(sampleCount > 0)
				{
					for(int i = 0; i < sampleCount; i++)
					{
						System.out.println(sampleBuffer[0][i]);
					}
					continue;
				}
				break;
			}
			catch (IOException e)
			{
				break;
			}
		}
	}
}
