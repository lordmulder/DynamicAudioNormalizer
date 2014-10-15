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

@FixMethodOrder(MethodSorters.NAME_ASCENDING)
public class DynamicAudioNormalizerTest
{
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
		for(int i = 0; i < 99; i++)
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
}
