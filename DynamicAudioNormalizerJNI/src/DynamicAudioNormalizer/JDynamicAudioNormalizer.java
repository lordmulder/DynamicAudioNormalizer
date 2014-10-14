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

	static synchronized int[] getVersionInfo()
	{
		int[] versionInfo = new int[3];
		
		try
		{
			if(!NativeAPI.getVersionInfo(versionInfo))
			{
				throw new Error("Failed to retrieve version info from native library!");
			}
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}
		
		return versionInfo;
	}
	
	static synchronized Map<String,String> getBuildInfo()
	{
		Map<String,String> buildInfo = new HashMap<String, String>();
		
		try
		{
			if(!NativeAPI.getBuildInfo(buildInfo))
			{
				throw new Error("Failed to retrieve build info from native library!");
			}
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}
		
		return buildInfo;
	}
	
	static synchronized void setLoggingHandler(final Logger logger)
	{
		try
		{
			if(!NativeAPI.setLoggingHandler(logger))
			{
				throw new Error("Failed to setup new logging handler!");
			}
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}
	}
	
	//------------------------------------------------------------------------------------------------
	// Constructor and Finalizer
	//------------------------------------------------------------------------------------------------

	private int m_instance = -1;
	
	JDynamicAudioNormalizer(final int channels, final int sampleRate, final int frameLenMsec, final int filterSize, final double peakValue, final double maxAmplification, final double targetRms, final double compressFactor, final boolean channelsCoupled, final boolean enableDCCorrection, final boolean altBoundaryMode)
	{
		try
		{
			m_instance = NativeAPI.createInstance(channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, channelsCoupled, enableDCCorrection, altBoundaryMode);
			if(m_instance < 0)
			{
				throw new Error("Failed to create native DynamicAudioNormalizer instance!");
			}
			System.out.println("Handle value: " + m_instance);
		}
		catch(Throwable e)
		{
			handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
		}
	}
	
	public synchronized void release()
	{
		if(m_instance >= 0)
		{
			try
			{
				final int instanceTmp = m_instance;
				m_instance = -1;
				if(!NativeAPI.destroyInstance(instanceTmp))
				{
					throw new Error("Failed to destroy native DynamicAudioNormalizer instance!");
				}
			}
			catch(Throwable e)
			{
				handleError(e, "Failed to call native DynamicAudioNormalizerAPI function!");
			}
		}
	}
	
	protected synchronized void finalize()
	{
		release();
	}
	
	//------------------------------------------------------------------------------------------------
	// Test Functions
	//------------------------------------------------------------------------------------------------
	
	private static void selfTest()
	{
		// setLogger()
		setLoggingHandler(new Logger()
		{
			public void log(int level, String message) {
				System.err.println("[JDynAudNorm] " + message);
			}
		});
		
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
		
		//Create instances
		for(int i = 0; i < 99999; i++)
		{
			// Constructor
			JDynamicAudioNormalizer instance = new JDynamicAudioNormalizer(2, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, true, false, false);
			
			// Release
			instance.release();
		}
	}
	
	public static void main(String[] args)
	{
		System.out.println("DynamicAudioNormalizer JNI Tester\n");
		
		try
		{
			selfTest();
			System.out.println("Completed successfully.");
		}
		catch(Exception e)
		{
			System.err.println("SOMETHING WENT WRONG !!!");
		}
	}
}
