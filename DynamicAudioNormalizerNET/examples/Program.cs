using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Reflection;

using DynamicAudioNormalizer;

namespace DynamicAudioNormalizerTest
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
                        samplesOut[c, i] = (double)m_reader.ReadInt16() / 32767.0;
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
                runTest();
            }
            catch (Exception e)
            {
                Console.WriteLine("\n" + e.GetType() + ": " + e.Message + "\n");
            }
        }

        private static void runTest()
        {
            Console.WriteLine("DynamicAudioNormalizer.NET Tester [" + Assembly.GetExecutingAssembly().GetName().Version.ToString() + "]\n");

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
                        Console.WriteLine("channels: " + channels);
                        Console.WriteLine("sampleRate: " + sampleRate);
                        Console.WriteLine("frameLen: " + frameLen);
                        Console.WriteLine("filterSize: " + filterSize);

                        long internalDelay = instance.getInternalDelay();
                        Console.WriteLine("internalDelay: " + internalDelay);

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
