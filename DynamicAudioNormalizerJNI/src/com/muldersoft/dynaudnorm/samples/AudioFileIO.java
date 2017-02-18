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

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class AudioFileIO {

	public enum PCMFileType
	{
		PCM_16BIT_LE,
		PCM_16BIT_BE,
		PCM_32BIT_LE,
		PCM_32BIT_BE,
		PCM_64BIT_LE,
		PCM_64BIT_BE,
	}
	
	private static class PCMFileBase {
		protected static ByteOrder getEndianess(final PCMFileType type) {
			switch(type) {
			case PCM_16BIT_LE:
			case PCM_32BIT_LE:
			case PCM_64BIT_LE:
				return ByteOrder.LITTLE_ENDIAN;
			case PCM_32BIT_BE:
			case PCM_16BIT_BE:
			case PCM_64BIT_BE:
				return ByteOrder.BIG_ENDIAN;
			default:
				throw new IllegalArgumentException();
			}
		}
		
		protected static int getSampleSize(final PCMFileType type) {
			switch(type) {
			case PCM_16BIT_LE:
			case PCM_16BIT_BE:
				return 2;
			case PCM_32BIT_LE:
			case PCM_32BIT_BE:
				return 4;
			case PCM_64BIT_LE:
			case PCM_64BIT_BE:
				return 8;
			default:
				throw new IllegalArgumentException();
			}
		}
		
		protected static double readSample(final PCMFileType type, final ByteBuffer input) {
			switch(type) {
			case PCM_16BIT_LE:
			case PCM_16BIT_BE:
				return ShortToDouble(input.getShort());
			case PCM_32BIT_LE:
			case PCM_32BIT_BE:
				return input.getFloat();
			case PCM_64BIT_LE:
			case PCM_64BIT_BE:
				return input.getDouble();
			default:
				throw new IllegalArgumentException();
			}
		}
		
		protected static ByteBuffer writeSample(final PCMFileType type, final ByteBuffer output, final double value) {
			switch(type) {
			case PCM_16BIT_LE:
			case PCM_16BIT_BE:
				return output.putShort(doubleToShort(value));
			case PCM_32BIT_LE:
			case PCM_32BIT_BE:
				return output.putFloat((float)value);
			case PCM_64BIT_LE:
			case PCM_64BIT_BE:
				return output.putDouble(value);
			default:
				throw new IllegalArgumentException();
			}
		}
		
		private static double ShortToDouble(final short val) {
			return ((double) val) / ((double) Short.MAX_VALUE);
		}
		
		private static short doubleToShort(final double val) {
			return (short) Math.round(Math.max(-1.0, Math.min(1.0, val)) * ((double) Short.MAX_VALUE));
		}
	}
	
	public static class PCMFileReader extends PCMFileBase implements AutoCloseable {
		private final PCMFileType type;
		private final int channels;
		private final BufferedInputStream inputStream;
		private byte[] tempBuffer = null;

		public PCMFileReader(final String fileName, final int channels, final PCMFileType type) throws IOException {
			if((channels < 1) || (channels > 8)) {
				throw new IllegalArgumentException("Invalid number of channels!");
			}
			this.type = type;
			this.channels = channels;
			inputStream = new BufferedInputStream(new FileInputStream(new File(fileName)));
		}

		public int read(double[][] buffer) throws IOException {
			if (buffer.length < channels) {
				throw new RuntimeException("Output array dimension must match channels!");
			}
			
			int buffSize = Integer.MAX_VALUE;
			for(int c = 0; c < channels; c++) {
				if((buffSize = Math.min(buffSize, buffer[c].length)) < 1) {
					throw new RuntimeException("Output array length must be at least one!");
				}
			}
			
			final int frameSize = getSampleSize(type) * channels;
			final int maxReadSize = buffSize * frameSize;
			if ((tempBuffer == null) || (tempBuffer.length < maxReadSize)) {
				tempBuffer = new byte[maxReadSize];
			}

			final int byteCount = inputStream.read(tempBuffer, 0, maxReadSize);
			if((byteCount % frameSize) != 0) {
				throw new RuntimeException("Number of bytes is not a multiple of frame size!");
			}
			
			final int sampleCount = byteCount / frameSize;
			if (sampleCount > 0) {
				ByteBuffer byteBuffer = ByteBuffer.wrap(tempBuffer);
				byteBuffer.order(getEndianess(type));
				for (int i = 0; i < sampleCount; i++) {
					for (int c = 0; c < channels; c++) {
						buffer[c][i] = readSample(type, byteBuffer);
					}
				}
			}

			return sampleCount;
		}

		public void close() throws IOException {
			inputStream.close();
		}
	}

	// ------------------------------------------------------------------------------------------------
	// PCM File Writer
	// ------------------------------------------------------------------------------------------------

	public static class PCMFileWriter extends PCMFileBase implements AutoCloseable {
		private final PCMFileType type;
		private final int channels;
		private final BufferedOutputStream outputStream;
		private byte[] tempBuffer = null;

		public PCMFileWriter(final String fileName, final int channels, final PCMFileType type) throws IOException {
			if((channels < 1) || (channels > 8)) {
				throw new IllegalArgumentException("Invalid number of channels!");
			}
			this.type = type;
			this.channels = channels;
			outputStream = new BufferedOutputStream(new FileOutputStream(new File(fileName)));
		}

		public void write(double[][] buffer, final int sampleCount) throws IOException {
			if (buffer.length < channels) {
				throw new IOException("Input array dimension must match channels!");
			}

			for(int c = 0; c < channels; c++) {
				if(buffer[c].length < sampleCount) {
					throw new RuntimeException("Input array length must be greater or equal to sampleCount!");
				}
			}
			
			final int frameSize = getSampleSize(type) * channels;
			final int writeSize = sampleCount * frameSize;
			if ((tempBuffer == null) || (tempBuffer.length < writeSize)) {
				tempBuffer = new byte[writeSize];
			}

			if (sampleCount > 0) {
				ByteBuffer byteBuffer = ByteBuffer.wrap(tempBuffer);
				byteBuffer.order(getEndianess(type));
				for (int i = 0; i < sampleCount; i++) {
					for (int c = 0; c < channels; c++) {
						writeSample(type, byteBuffer, buffer[c][i]);
					}
				}
			}

			outputStream.write(tempBuffer, 0, writeSize);
		}

		public void flush() throws IOException {
			outputStream.flush();
		}

		public void close() throws IOException {
			flush();
			outputStream.close();
		}
	}

}
