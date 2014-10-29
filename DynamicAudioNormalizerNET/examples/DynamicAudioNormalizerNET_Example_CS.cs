//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Microsoft.NET Wrapper
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

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Reflection;

using DynamicAudioNormalizer;

namespace DynamicAudioNormalizer_Example
{
    class SampleReader : IDisposable
    {
        public SampleReader(String fileName)
        {
            m_reader = new BinaryReader(File.OpenRead(fileName));
        }

        public int read(double[,] samplesOut)
        {
            int channels   = samplesOut.GetLength(0);
            int maxSamples = samplesOut.GetLength(1);

            for (int i = 0; i < maxSamples; i++)
            {
                for (int c = 0; c < channels; c++)
                {
                    try
                    {
                        samplesOut[c, i] = (double) m_reader.ReadInt16() / 32767.0;
                    }
                    catch(EndOfStreamException)
                    {
                        return i;
                    }
                }
            }

            return maxSamples;
        }

        public void Dispose()
        {
            m_reader.Dispose();
        }

        private readonly BinaryReader m_reader;
    }

    class SampleWriter : IDisposable
    {
        public SampleWriter(String fileName)
        {
            m_writer = new BinaryWriter(File.OpenWrite(fileName));
        }

        public void write(double[,] samplesIn, int sampleCount)
        {
            int channels = samplesIn.GetLength(0);

            for (int i = 0; i < sampleCount; i++)
            {
                for (int c = 0; c < channels; c++)
                {
                    m_writer.Write((short)(samplesIn[c, i] * 32767.0));
                }
            }
        }

        public void Dispose()
        {
            m_writer.Dispose();
        }

        private readonly BinaryWriter m_writer;
    }

    class Program
    {
        static void Main(string[] args)
        {
            try
            {
                String version = Assembly.GetExecutingAssembly().GetName().Version.ToString();
                Console.WriteLine("DynamicAudioNormalizer.NET C# Example [{0}]\n", version);
                runTest();
                GC.WaitForPendingFinalizers();
            }
            catch (Exception e)
            {
                Console.WriteLine("\nFAILURE!!!\n\n" + e.GetType() + ": " + e.Message + "\n");
            }
        }

        private static void runTest()
        {
            uint major, minor, patch;
            DynamicAudioNormalizerNET.getVersionInfo(out major, out minor, out patch);
            Console.WriteLine("Library Version {0}.{1}-{2}", major.ToString(), minor.ToString("D2"), minor, patch.ToString());

            String date, time, compiler, arch; bool debug;
            DynamicAudioNormalizerNET.getBuildInfo(out date, out time, out compiler, out arch, out debug);
            Console.WriteLine("Date: {0}, Time: {1}, Compiler: {2}, Arch: {3}, {4}", date, time, compiler, arch, debug ? "DEBUG" : "Release");

            DynamicAudioNormalizerNET.setLogger(delegate(int level, String message)
            {
                switch (level)
                {
                    case  0: Console.WriteLine("[DynAudNorm][DBG] " + message); break;
                    case  1: Console.WriteLine("[DynAudNorm][WRN] " + message); break;
                    case  2: Console.WriteLine("[DynAudNorm][ERR] " + message); break;
                    default: Console.WriteLine("[DynAudNorm][???] " + message); break;
                }
            });

            using(DynamicAudioNormalizerNET instance = new DynamicAudioNormalizerNET(2, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, true, false, false))
            {
                using (SampleReader reader = new SampleReader("Input.pcm"))
                {
                    using (SampleWriter writer = new SampleWriter("Output.pcm"))
                    {
                        uint channels, sampleRate, frameLen, filterSize;
                        instance.getConfiguration(out channels, out sampleRate, out frameLen, out filterSize);
                        Console.WriteLine("channels: " + channels.ToString());
                        Console.WriteLine("sampleRate: " + sampleRate.ToString());
                        Console.WriteLine("frameLen: " + frameLen.ToString());
                        Console.WriteLine("filterSize: " + filterSize.ToString());

                        long internalDelay = instance.getInternalDelay();
                        Console.WriteLine("internalDelay: " + internalDelay.ToString());

                        process(reader, writer, instance);
                    }
                }
            }
        }

        private static void process(SampleReader reader, SampleWriter writer, DynamicAudioNormalizerNET instance)
        {
            double[,] data = new double[2, 4096];

            Console.WriteLine("Processing audio samples in-place...");
            for(;;)
            {
                int inputSamples = reader.read(data);
                if(inputSamples > 0)
                {
                    int outputSamples = (int)instance.processInplace(data, inputSamples);
                    if(outputSamples > 0)
                    {
                        writer.write(data, outputSamples);
                    }
                }
                if(inputSamples < data.GetLength(1))
                {
                    break; /*EOF*/
                }
            }

            Console.WriteLine("Flushing remaining samples from buffer...");
            for(;;)
            {
                int outputSamples = (int) instance.flushBuffer(data);
                if (outputSamples > 0)
                {
                    writer.write(data, outputSamples);
                }
                if (outputSamples < 1)
                {
                    break; /*EOF*/
                }
            }

            Console.WriteLine("Resetting the normalizer instance...");
            instance.reset(); //Just for testing, this NOT actually required here!

            Console.WriteLine("Finished.");
        }
    }
}
