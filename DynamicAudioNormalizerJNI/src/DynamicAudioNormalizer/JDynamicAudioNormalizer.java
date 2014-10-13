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

package DynamicAudioNormalizer;

import java.util.HashMap;
import java.util.Map;

public class JDynamicAudioNormalizer
{
	//------------------------------------------------------------------------------------------------
	// Exception class
	//------------------------------------------------------------------------------------------------
	
	public static class Error extends RuntimeException
	{
		private static final long serialVersionUID = 8560360516304094422L;

		public Error(String string)
		{
			super(string);
		}
	}

	//------------------------------------------------------------------------------------------------
	// Initialization
	//------------------------------------------------------------------------------------------------

	static
	{
		try
		{
			System.loadLibrary("DynamicAudioNormalizerAPI"); // Load native library at runtime
		}
		catch(UnsatisfiedLinkError e)
		{
			System.err.println(e.getMessage());
			System.err.println("ERROR: Failed to load native DynamicAudioNormalizerAPI library!\n");
			throw new Error("Failed to load DynamicAudioNormalizerAPI library");
		}
	}

	//------------------------------------------------------------------------------------------------
	// Native API
	//------------------------------------------------------------------------------------------------
	
	private static class NativeAPI
	{
		private native static boolean getVersionInfo(int versionInfo[]);
		private native static boolean getBuildInfo(Map<String,String> buildInfo);
	}

	//------------------------------------------------------------------------------------------------
	// Public API
	//------------------------------------------------------------------------------------------------

	static int[] getVersionInfo()
	{
		int[] versionInfo = new int[3];
		
		try
		{
			if(!NativeAPI.getVersionInfo(versionInfo))
			{
				throw new RuntimeException("Failed to retrieve version info from native library!");
			}
		}
		catch(UnsatisfiedLinkError e)
		{
			System.err.println(e.getMessage());
			System.err.println("ERROR: Failed to call native DynamicAudioNormalizerAPI function!\n");
			throw new Error("Failed to call native function!");
		}
		
		return versionInfo;
	}
	
	static Map<String,String> getBuildInfo()
	{
		Map<String,String> buildInfo = new HashMap<String, String>();
		
		try
		{
			if(!NativeAPI.getBuildInfo(buildInfo))
			{
				throw new RuntimeException("Failed to retrieve build info from native library!");
			}
		}
		catch(UnsatisfiedLinkError e)
		{
			System.err.println(e.getMessage());
			System.err.println("ERROR: Failed to call native DynamicAudioNormalizerAPI function!\n");
			throw new Error("Failed to call native function!");
		}
		
		return buildInfo;
	}
	
	//------------------------------------------------------------------------------------------------
	// Test Functions
	//------------------------------------------------------------------------------------------------
	
	public static void main(String[] args)
	{
		System.out.println("DynamicAudioNormalizer JNI Tester\n");
	
		// getVersionInfo()
		final int[] versionInfo = JDynamicAudioNormalizer.getVersionInfo();
		System.out.println("Library Version: " + versionInfo[0] + "." + versionInfo[1] + "-" + versionInfo[2]);
		
		// getBuildInfo()
		final Map<String,String> buildInfo = JDynamicAudioNormalizer.getBuildInfo();
		for(final String key : buildInfo.keySet())
		{
			System.out.println("Build Info: " + key + "=" + buildInfo.get(key));
		}
		System.out.println();
		
		// Completed
		System.out.println("Completed.");
	}
}
