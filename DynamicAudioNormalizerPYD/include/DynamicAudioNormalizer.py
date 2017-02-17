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

#import the "C" module
import DynamicAudioNormalizerAPI 

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
	
	def processInplace(self):
		print("processInplace()")
		if not self._instance:
			raise RuntimeError("Instance not initialized!")
		return DynamicAudioNormalizerAPI.processInplace(self._instance, samplesInOut, count)
	
	@staticmethod
	def getVersion():
		return DynamicAudioNormalizerAPI.getVersion()

print(DynamicAudioNormalizer)
print(DynamicAudioNormalizer.getVersion())

with DynamicAudioNormalizer(2,44100) as test:
	print(test)
	x = test.getConfig()
	print(x)

