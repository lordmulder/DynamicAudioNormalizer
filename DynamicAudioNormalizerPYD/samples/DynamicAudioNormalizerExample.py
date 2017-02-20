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
import os
import datetime
import array

#import PyDynamicAudioNormalizer
from DynamicAudioNormalizer import PyDynamicAudioNormalizer

#import Wave Reader and Writer
from WaveFileUtils import WaveFileReader
from WaveFileUtils import WaveFileWriter


#----------------------------------------------------------------------
# Utility Functions
#----------------------------------------------------------------------

def alloc_sample_buffers(channles, length):
	buffers = ()
	for i in range(0, channles):
		arr = array.array('d')
		for j in range(0, length):
			arr.append(0.0)
		buffers += (arr,)
	return buffers


#----------------------------------------------------------------------
# Logging Callback
#----------------------------------------------------------------------

logfile = None

def my_log_func(level, message):
	if logfile:
		logfile.write("[{}][{}] {}\n".format(datetime.datetime.utcnow().isoformat(), int(level), message))
		logfile.flush()


#----------------------------------------------------------------------
# Processing Loop
#----------------------------------------------------------------------

def normalize_samples(wav_reader, wav_writer, logfile, config):
	print(" Done!\nInitializing...", end = '', flush = True)
	if logfile:
		PyDynamicAudioNormalizer.setLogFunction(my_log_func)
	with PyDynamicAudioNormalizer(wav_reader.getChannels(), wav_reader.getSamplerate(), *config) as normalizer:
		indicator, buffersIn, buffersOut = 0, alloc_sample_buffers(wav_reader.getChannels(), 4096), alloc_sample_buffers(wav_reader.getChannels(), 4096)
		print(" Done!\nProcessing audio samples...", end = '', flush = True)
		while True:
			count = wav_reader.read(buffersIn)
			if count:
				count = normalizer.process(buffersIn, buffersOut, count)
				if count:
					wav_writer.write(buffersOut, count)
				indicator += 1
				if indicator >= 7:
					print('.', end = '', flush = True)
					indicator = 0
				continue
			break;
		print(" Done!\nFlushing buffers...", end = '', flush = True)
		while True:
			count = normalizer.flushBuffer(buffersOut)
			if count:
				wav_writer.write(buffersOut, count)
				indicator += 1
				if indicator >= 7:
					print('.', end = '', flush = True)
					indicator = 0
				continue
			break;
		print(' Done!\n')


#----------------------------------------------------------------------
# Main
#----------------------------------------------------------------------

version = PyDynamicAudioNormalizer.getVersion()
print("Dynamic Audio Normalizer, Version {}.{}-{} [{}]".format(*version[0], version[1][4]))
print("Copyright (c) 2014-{} LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.".format(version[1][0][7:]))
print("Built on {} at {} with {} for Python-{}.\n".format(*version[1]))

if (len(sys.argv) >= 5):
	config = (int(sys.argv[1]), int(sys.argv[2]))
	source_file = os.path.abspath(sys.argv[3])
	output_file = os.path.abspath(sys.argv[4])
elif (len(sys.argv) >= 4):
	config = (int(sys.argv[1]),)
	source_file = os.path.abspath(sys.argv[2])
	output_file = os.path.abspath(sys.argv[3])
elif (len(sys.argv) >= 3):
	config = ()
	source_file = os.path.abspath(sys.argv[1])
	output_file = os.path.abspath(sys.argv[2])
else:
	print("Usage:\n  {} {} [[<filterSize>] frameLen] <input.wav> <output.wav>\n".format(os.path.basename(sys.executable), os.path.basename(__file__)), file=sys.stderr)
	sys.exit(1)

if not os.path.isfile(source_file):
	print("Input file \"{}\" not found!\n".format(source_file), file=sys.stderr)
	sys.exit(1)

print("Source file: \"{}\"".format(source_file))
print("Output file: \"{}\"".format(output_file))

logfilePath = os.environ.get('LOGFILE') 
if logfilePath:
	print("Report file: \"{}\"".format(logfilePath))
	logfile = open(logfilePath, 'w')
else:
	logfile = None

print("\nOpening input and output files...", end = '', flush = True)
with WaveFileReader(source_file) as wav_reader:
	with WaveFileWriter(output_file, wav_reader.getChannels(), wav_reader.getSampleWidth(), wav_reader.getSamplerate()) as wav_writer:
		normalize_samples(wav_reader, wav_writer, logfile, config)
		print('All is done. Goodbye.')

if logfile:
	logfile.close()
