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

package com.muldersoft.dynaudnorm;

import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.Queue;

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

	private static void handleError(final Throwable e, final String message)
	{
		try
		{
			System.err.println(String.format("ERROR: %s\n%s: \"%s\"\n", message, e.getClass().getName(), e.getMessage()));
		}
		catch(Throwable e2)
		{
			/*discard any error that may occur inside the error handler*/
		}
		throw new Error(message);
	}
	
	//------------------------------------------------------------------------------------------------
	// Logging interface
	//------------------------------------------------------------------------------------------------
	
	public static abstract class Logger
	{
		public abstract void log(final int level, final String message);
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
		catch(Throwable e)
		{
			handleError(e, "Failed to load native DynamicAudioNormalizerAPI library!");
		}
	}

	//------------------------------------------------------------------------------------------------
	// Native API
	//------------------------------------------------------------------------------------------------
	
	private static class NativeAPI
	{
		//Static Functions
		private native static boolean getVersionInfo(final int versionInfo[]);
		private native static boolean getBuildInfo(final Map<String,String> buildInfo);
		private native static boolean setLoggingHandler(final Logger logger);

		//Create or Destroy Instance
		private native static int createInstance(final int channels, final int sampleRate, final int frameLenMsec, final int filterSize, final double peakValue, final double maxAmplification, final double targetRms, final double compressFactor, final boolean channelsCoupled, final boolean enableDCCorrection, final boolean altBoundaryMode);
		private native static boolean destroyInstance(final int handle);
	}

	//------------------------------------------------------------------------------------------------
	// Public API
	//------------------------------------------------------------------------------------------------

	public static synchronized int[] getVersionInfo()
	{
		boolean success = false;
		int[] versionInfo = new int[3];
		
		try
		{
			success = NativeAPI.getVersionInfo(versionInfo);
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}

		if(!success)
		{
			throw new Error("Failed to retrieve version info from native library!");
		}
		
		return versionInfo;
	}
	
	public static synchronized Map<String,String> getBuildInfo()
	{
		boolean success = false;
		Map<String,String> buildInfo = new HashMap<String, String>();
		
		try
		{
			success = NativeAPI.getBuildInfo(buildInfo);
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}

		if(!success)
		{
			throw new Error("Failed to retrieve build info from native library!");
		}
		
		return buildInfo;
	}
	
	public static synchronized void setLoggingHandler(final Logger logger)
	{
		boolean success = false;
		
		try
		{
			success = NativeAPI.setLoggingHandler(logger);
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}
		
		if(!success)
		{
			throw new Error("Failed to setup new logging handler!");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Constructor and Finalizer
	//------------------------------------------------------------------------------------------------

	private int m_instance = -1;
	
	public JDynamicAudioNormalizer(final int channels, final int sampleRate, final int frameLenMsec, final int filterSize, final double peakValue, final double maxAmplification, final double targetRms, final double compressFactor, final boolean channelsCoupled, final boolean enableDCCorrection, final boolean altBoundaryMode)
	{
		try
		{
			m_instance = NativeAPI.createInstance(channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, channelsCoupled, enableDCCorrection, altBoundaryMode);
			System.out.println("Handle value: " + m_instance);
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}

		if(m_instance < 0)
		{
			throw new Error("Failed to create native DynamicAudioNormalizer instance!");
		}
	}
	
	public synchronized void release()
	{
		if(m_instance >= 0)
		{
			boolean success = false;
			try
			{
				final int instanceTmp = m_instance;
				m_instance = -1;
				success = NativeAPI.destroyInstance(instanceTmp);
			}
			catch(Throwable e)
			{
				handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
			}
			
			if(!success)
			{
				throw new Error("Failed to destroy native DynamicAudioNormalizer instance!");
			}
		}
	}
	
	protected synchronized void finalize()
	{
		release();
	}
}
