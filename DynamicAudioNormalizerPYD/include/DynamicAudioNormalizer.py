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
import array
import struct
import wave

#import the "C" module
import DynamicAudioNormalizerAPI 


#----------------------------------------------------------------------
# DynamicAudioNormalizer
#----------------------------------------------------------------------

class DynamicAudioNormalizer:
	"""Wrapper class around the DynamicAudioNormalizerAPI library"""
	
	def __str__(self):
		versionInfo = self.getVersion()
		buildStr = "built: {} {}, compiler: {}, arch: {}, {}".format(*versionInfo[1])
		configStr = "channels={}, samplingRate={}, frameLen={}, filterSize={}".format(*self.getConfig())
		return "DynamicAudioNormalizer v{}.{}-{} [{}][{}]]".format(*versionInfo[0], buildStr, configStr)
	
	def __init__(self,channels,samplerate):
		print("__init__")
		self._channels = channels
		self._samplerate = samplerate
		self._instance = None
	
	def __enter__(self):
		print("__enter__")
		self._instance = DynamicAudioNormalizerAPI.create(self._channels, self._samplerate)
		print(self._instance)
		return self
	
	def __exit__(self, type, value, traceback):
		print("__exit__")
		if not self._instance:
			raise RuntimeError("Instance not initialized!")
		DynamicAudioNormalizerAPI.destroy(self._instance)
		self._instance = None
	
	def __del__(self):
		print("__del__")
		if self._instance:
			print("RESOURCE LEAK: DynamicAudioNormalizer object was not de-initialized properly!", file=sys.stderr)
	
	def getConfig(self):
		print("getConfig()")
		if not self._instance:
			raise RuntimeError("Instance not initialized!")
		return DynamicAudioNormalizerAPI.getConfig(self._instance)
	
	def processInplace(self, samplesInOut, count):
		print("processInplace()")
		if not self._instance:
			raise RuntimeError("Instance not initialized!")
		return DynamicAudioNormalizerAPI.processInplace(self._instance, samplesInOut, count)
	
	@staticmethod
	def getVersion():
		return DynamicAudioNormalizerAPI.getVersion()


#----------------------------------------------------------------------
# WaveAudioReader
#----------------------------------------------------------------------

class _WaveFileBase:
	def _unpack_samples(self, samples, buffers, channels, sampwidth):
		cidx, sidx = 0, 0
		type = self._get_sample_type(sampwidth)
		for sample in struct.iter_unpack(type[0], samples):
			buffers[cidx][sidx] = sample[0] / type[1]
			cidx += 1
			if cidx >= channels:
				cidx, sidx = 0, sidx + 1
		return sidx
	
	def _minimum_buff_length(self, buffers):
		result = sys.maxsize
		for b in buffers:
			result = min(result, len(b))
		if result < 1:
			raise ValueError("Buffer length is zero!")
		return result
	
	def _get_sample_type(self, sampwidth):
		if sampwidth == 1:
			return ('B', 0xFF)
		elif sampwidth == 2:
			return ('<h', 0x7FFF)
		elif sampwidth == 4:
			return ('<f', 1.0)
		elif sampwidth == 8:
			return ('<d', 1.0)
		else:
			raise ValueError("Unknown sample size!")


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
# Utility Functions
#----------------------------------------------------------------------

def alloc_buffers(channles, length):
	buffers = ()
	for i in range(0, channles):
		arr = array.array('d')
		for j in range(0, length):
			arr.append(0.0)
		buffers += (arr,)
	return buffers


#----------------------------------------------------------------------
# Main
#----------------------------------------------------------------------

print(DynamicAudioNormalizer)
print(DynamicAudioNormalizer.getVersion())

with WaveFileReader('The Root of All Evil.wav') as wav_reader:
	with DynamicAudioNormalizer(wav_reader.getChannels(), wav_reader.getSamplerate()) as normalizer:
		buffers = alloc_buffers(wav_reader.getChannels(), 4096)
		while True:
			count = wav_reader.read(buffers)
			if count:
				print(normalizer.processInplace(buffers, count))
				continue
			break;
