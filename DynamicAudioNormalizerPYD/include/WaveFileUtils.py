##################################################################################
# Dynamic Audio Normalizer - Python Wrapper
# Copyright (c) 2014-2017 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# http://opensource.org/licenses/MIT
##################################################################################

#import stdlib modules
import sys
import math
import struct
import wave


#----------------------------------------------------------------------
# Wave Audio Base
#----------------------------------------------------------------------

class _WaveFileBase:
	"""Base class for Wave file sample processing"""
	_THRESHOLD = math.sqrt(2.0)
	
	def _unpack_samples(self, samples, buffers, channels, sampwidth):
		cidx, sidx = 0, 0
		type = self._get_sample_type(sampwidth)
		for sample in struct.iter_unpack(type[0], samples):
			if type[1] > self._THRESHOLD:
				buffers[cidx][sidx] = float(sample[0]) / float(type[1])
			else:
				buffers[cidx][sidx] = float(sample[0])
			cidx += 1
			if cidx >= channels:
				cidx, sidx = 0, sidx + 1
		return sidx
	
	def _repack_samples(self, buffers, channels, sampwidth, length):
		type = self._get_sample_type(sampwidth)
		samples = bytearray(channels * length * sampwidth)
		offset = 0
		for sidx in range(0, length):
			for cidx in range(0, channels):
				if type[1] > self._THRESHOLD:
					value = int(round(buffers[cidx][sidx] * type[1]))
				else:
					value = buffers[cidx][sidx]
				struct.pack_into(type[0], samples, offset, value)
				offset += sampwidth
		return samples
	
	def _minimum_buff_length(self, buffers):
		result = sys.maxsize
		for b in buffers:
			result = min(result, len(b))
		if result < 1:
			raise ValueError("Buffer length is zero!")
		return result
	
	def _get_sample_type(self, sampwidth):
		if sampwidth == 2:
			return ('<h', 0x7FFF)
		elif sampwidth == 4:
			return ('<f', 1.0)
		elif sampwidth == 8:
			return ('<d', 1.0)
		else:
			raise ValueError("Unknown sample size!")


#----------------------------------------------------------------------
# Wave Audio Writer
#----------------------------------------------------------------------

class WaveFileReader(_WaveFileBase):
	"""Helper class to read samples from Wave file"""
	
	def __init__(self,filename):
		self._filename = filename
		self._wavefile = None
		self._channels = None
		self._samplewidth = None
		self._samplerate = None
	
	def __enter__(self):
		self._wavefile = wave.open(self._filename, 'rb')
		if self._wavefile:
			self._channels = self._wavefile.getnchannels()
			self._samplewidth =  self._wavefile.getsampwidth()
			self._samplerate = self._wavefile.getframerate()
		return self
	
	def __exit__(self, type, value, traceback):
		if not self._wavefile:
			raise RuntimeError("Instance not initialized!")
		self._wavefile.close()
		self._wavefile = None
		self._channels = None
		self._samplewidth = None
		self._samplerate = None
	
	def __del__(self):
		if self._wavefile:
			print("RESOURCE LEAK: WaveFileReader object was not de-initialized properly!", file=sys.stderr)
			
	def getChannels(self):
		if not self._wavefile:
			raise RuntimeError("Instance not initialized!")
		return self._channels
		
	def getSamplerate(self):
		if not self._wavefile:
			raise RuntimeError("Instance not initialized!")
		return self._samplerate
		
	def getSampleWidth(self):
		if not self._wavefile:
			raise RuntimeError("Instance not initialized!")
		return self._samplewidth
		
	def read(self, buffers):
		if not self._wavefile:
			raise RuntimeError("Instance not initialized!")
		if len(buffers) < self._channels:
			raise RuntimeError("Number of buffers is insufficient!")
		frames = self._wavefile.readframes(self._minimum_buff_length(buffers))
		if frames:
			return self._unpack_samples(frames, buffers, self._channels, self._samplewidth)
		return None


#----------------------------------------------------------------------
# Wave Audio Writer
#----------------------------------------------------------------------

class WaveFileWriter(_WaveFileBase):
	"""Helper class to write samples to Wave file"""
	
	def __init__(self,filename,channels,samplewidth,samplerate):
		self._filename = filename
		self._wavefile = None
		self._channels = channels
		self._samplewidth = samplewidth
		self._samplerate = samplerate
	
	def __enter__(self):
		self._wavefile = wave.open(self._filename, 'wb')
		if self._wavefile:
			self._wavefile.setnchannels(self._channels)
			self._wavefile.setsampwidth(self._samplewidth)
			self._wavefile.setframerate(self._samplerate)
		return self
	
	def __exit__(self, type, value, traceback):
		if not self._wavefile:
			raise RuntimeError("Instance not initialized!")
		self._wavefile.close()
		self._wavefile = None
		self._channels = None
		self._samplewidth = None
		self._samplerate = None
	
	def __del__(self):
		if self._wavefile:
			print("RESOURCE LEAK: WaveFileWriter object was not de-initialized properly!", file=sys.stderr)
			
	def getChannels(self):
		if not self._wavefile:
			raise RuntimeError("Instance not initialized!")
		return self._channels
		
	def getSamplerate(self):
		if not self._wavefile:
			raise RuntimeError("Instance not initialized!")
		return self._samplerate
		
	def getSampleWidth(self):
		if not self._wavefile:
			raise RuntimeError("Instance not initialized!")
		return self._samplewidth
		
	def write(self, buffers, length):
		if not self._wavefile:
			raise RuntimeError("Instance not initialized!")
		if len(buffers) < self._channels:
			raise RuntimeError("Number of buffers is insufficient!")
		if length < 1:
			raise RuntimeError("The given length value is zero!")
		if length > self._minimum_buff_length(buffers):
			raise RuntimeError("Length exceeds size of buffer!")
		samples = self._repack_samples(buffers, self._channels, self._samplewidth, length)
		if samples:
			return self._wavefile.writeframes(samples)
		return None

