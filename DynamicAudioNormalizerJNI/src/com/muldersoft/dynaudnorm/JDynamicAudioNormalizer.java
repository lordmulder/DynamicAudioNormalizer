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

package com.muldersoft.dynaudnorm;

import java.io.Closeable;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class JDynamicAudioNormalizer implements Closeable
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
	// Logging interface
	//------------------------------------------------------------------------------------------------
	
	public static abstract class Logger
	{
		public abstract void log(final int level, final String message);
	}

	//------------------------------------------------------------------------------------------------
	// Native API
	//------------------------------------------------------------------------------------------------
	
	private static class NativeAPI_r7
	{
		static NativeAPI_r7 nativeAPI = null;
		
		//Singelton
		public static synchronized NativeAPI_r7 getInstance()
		{
			if(nativeAPI == null)
			{
				nativeAPI = new NativeAPI_r7();
			}
			return nativeAPI;
		}
		
		//Load native library at runtime
		private NativeAPI_r7()
		{
			try
			{
				final int coreVersion = getCoreVersion();
				System.loadLibrary((coreVersion > 0) ? String.format("DynamicAudioNormalizerAPI-%d", coreVersion) : "DynamicAudioNormalizerAPI");
			}
			catch(Throwable e)
			{
				handleError(e, "Failed to load native DynamicAudioNormalizerAPI library!");
			}
		}
		
		//Detect the library "core" version
		private int getCoreVersion()
		{
			int coreVersion = -1;
			try
			{
				if(!System.getProperty("os.name").toLowerCase().startsWith("win"))
				{
					final Pattern pattern = Pattern.compile(".+\\$NativeAPI_r(\\d+)$");
					final Matcher matcher = pattern.matcher(this.getClass().getName());
					if(matcher.matches())
					{
						coreVersion = Integer.parseInt(matcher.group(1));
					}
				}
			}
			catch(Throwable e)
			{
				handleError(e, "Failed to determine DynamicAudioNormalizerAPI core version!");
			}
			return coreVersion;
		}
		
		//Static Functions
		private native boolean getVersionInfo(final int versionInfo[]);
		private native boolean getBuildInfo(final Map<String,String> buildInfo);
		private native boolean setLoggingHandler(final Logger logger);

		//Create or Destroy Instance
		private native int createInstance(final int channels, final int sampleRate, final int frameLenMsec, final int filterSize, final double peakValue, final double maxAmplification, final double targetRms, final double compressFactor, final boolean channelsCoupled, final boolean enableDCCorrection, final boolean altBoundaryMode);
		private native boolean destroyInstance(final int handle);
		
		//Processing Functions
		private native long processInplace(final int handle, final double [][] samplesInOut, final long inputSize);
		private native long flushBuffer(final int handle, final double [][] samplesOut);
		private native boolean reset(final int handle);
		
		//Other Functions
		private native boolean getConfiguration(final int handle, final Map<String,Integer> buildInfo);
		private native long getInternalDelay(final int handle);
	}
	
	//------------------------------------------------------------------------------------------------
	// Static Functions
	//------------------------------------------------------------------------------------------------

	public static synchronized int[] getVersionInfo()
	{
		boolean success = false;
		int[] versionInfo = new int[3];
		
		try
		{
			success = NativeAPI_r7.getInstance().getVersionInfo(versionInfo);
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
			success = NativeAPI_r7.getInstance().getBuildInfo(buildInfo);
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
			success = NativeAPI_r7.getInstance().setLoggingHandler(logger);
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
	// Constructor, Close() and Finalizer
	//------------------------------------------------------------------------------------------------

	private int m_instance = -1;
	
	public JDynamicAudioNormalizer(final int channels, final int sampleRate, final int frameLenMsec, final int filterSize, final double peakValue, final double maxAmplification, final double targetRms, final double compressFactor, final boolean channelsCoupled, final boolean enableDCCorrection, final boolean altBoundaryMode)
	{
		try
		{
			m_instance = NativeAPI_r7.getInstance().createInstance(channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, channelsCoupled, enableDCCorrection, altBoundaryMode);
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
	
	public synchronized void close()
	{
		if(m_instance >= 0)
		{
			boolean success = false;
			try
			{
				try
				{
					success = NativeAPI_r7.getInstance().destroyInstance(m_instance);
				}
				finally
				{
					m_instance = -1;
				}
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
		close();
	}
	
	//------------------------------------------------------------------------------------------------
	// Processing Functions
	//------------------------------------------------------------------------------------------------

	public synchronized long processInplace(final double [][] samplesInOut, final long inputSize)
	{
		checkInstance(); /*make sure we are properly initialized*/

		long outputSize = 0;
		try
		{
			outputSize = NativeAPI_r7.getInstance().processInplace(m_instance, samplesInOut, inputSize);
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}	
		
		if(outputSize < 0)
		{
			throw new Error("Failed to process the audio samples inplace!");
		}

		return outputSize;
	}
	
	public synchronized long flushBuffer(final double [][] samplesOut)
	{
		checkInstance(); /*make sure we are properly initialized*/

		long outputSize = 0;
		try
		{
			outputSize = NativeAPI_r7.getInstance().flushBuffer(m_instance, samplesOut);
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}	
		
		if(outputSize < 0)
		{
			throw new Error("Failed to flush samples from buffer!");
		}

		return outputSize;
	}
	
	public synchronized void reset()
	{
		checkInstance(); /*make sure we are properly initialized*/

		boolean success = false;
		try
		{
			success = NativeAPI_r7.getInstance().reset(m_instance);
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}	
		
		if(!success)
		{
			throw new Error("Failed to reset DynamicAudioNormalizerAPI instance!");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Other Functions
	//------------------------------------------------------------------------------------------------

	public synchronized Map<String,Integer> getConfiguration()
	{
		checkInstance(); /*make sure we are properly initialized*/
		
		boolean success = false;
		Map<String,Integer> configuration = new HashMap<String,Integer>();
		
		try
		{
			success = NativeAPI_r7.getInstance().getConfiguration(m_instance, configuration);
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}

		if(!success)
		{
			throw new Error("Failed to retrieve DynamicAudioNormalizerAPI configuration!");
		}
		
		return configuration;
	}
	
	public synchronized long getInternalDelay()
	{
		checkInstance(); /*make sure we are properly initialized*/
		
		long internalDelay = -1;
		try
		{
			internalDelay = NativeAPI_r7.getInstance().getInternalDelay(m_instance);
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}

		if(internalDelay < 0)
		{
			throw new Error("Failed to retrieve DynamicAudioNormalizerAPI internal delay!");
		}
		
		return internalDelay;
	}
	
	@Deprecated
	public synchronized int getHandle()
	{
		return m_instance;
	}
	
	//------------------------------------------------------------------------------------------------
	// Internal functions
	//------------------------------------------------------------------------------------------------

	private static void handleError(final Throwable e, final String message)
	{
		if(e.getClass() == Error.class)
		{
			throw (Error) e; /*pass through our own exceptions*/
		}
		else
		{
			throw new Error(String.format("%s [Reason: %s, Message: \"%s\"]", message, e.getClass().getName(), e.getMessage()));
		}
	}

	private void checkInstance()
	{
		if(m_instance < 0)
		{
			throw new Error("Native DynamicAudioNormalizer object not created yet or released already!");
		}
	}
	//------------------------------------------------------------------------------------------------
	// Main
	//------------------------------------------------------------------------------------------------

	public static void main(String [] args)
	{
		int [] ver = null;
		try
		{
			ver = getVersionInfo();
		}
		catch(Throwable e)
		{
			System.out.printf("\nERROR: %s\n\n", e.getMessage());
		}
		
		System.out.println("===========================================================================");
		if(ver != null)
		{
			System.out.printf("Dynamic Audio Normalizer, Version %d.%02d-%d\n", ver[0], ver[1], ver[2]);
		}
		else
		{
			System.out.println("Dynamic Audio Normalizer, Unknown Version");
		}
		System.out.println("Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.");
		System.out.println();
		System.out.println("This program is free software: you can redistribute it and/or modify");
		System.out.println("it under the terms of the GNU General Public License <http://www.gnu.org/>.");
		System.out.println("Note that this program is distributed with ABSOLUTELY NO WARRANTY.");
		System.out.println("===========================================================================");
	}
}
