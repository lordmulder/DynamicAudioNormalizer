REM //////////////////////////////////////////////////////////////////////////////////
REM // Dynamic Audio Normalizer - Microsoft.NET Wrapper
REM // Copyright (c) 2014-2018 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.
REM //
REM // Permission is hereby granted, free of charge, to any person obtaining a copy
REM // of this software and associated documentation files (the "Software"), to deal
REM // in the Software without restriction, including without limitation the rights
REM // to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
REM // copies of the Software, and to permit persons to whom the Software is
REM // furnished to do so, subject to the following conditions:
REM //
REM // The above copyright notice and this permission notice shall be included in
REM // all copies or substantial portions of the Software.
REM //
REM // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
REM // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
REM // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
REM // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
REM // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
REM // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
REM // THE SOFTWARE.
REM //
REM // http://opensource.org/licenses/MIT
REM //////////////////////////////////////////////////////////////////////////////////

Imports System.Reflection
Imports DynamicAudioNormalizer

Module DynamicAudioNormalizerExample

    REM --------------------------------------------------------------------------
    REM AudioFileReader Class
    REM --------------------------------------------------------------------------
    Class AudioFileReader
        Implements IDisposable
        Private Reader As IO.BinaryReader

        Public Sub New(FileName As String)
            Reader = New IO.BinaryReader(IO.File.OpenRead(FileName))
        End Sub

        Protected Sub Dispose() Implements IDisposable.Dispose
            If Reader IsNot Nothing Then
                Reader.Dispose()
            End If
        End Sub

        Public Function Read(SamplesOut(,) As Double) As Long
            Dim Channels As Integer = SamplesOut.GetLength(0)
            Dim MaxSamples As Integer = SamplesOut.GetLength(1)
            For I As Long = 0 To MaxSamples - 1
                For C As Long = 0 To Channels - 1
                    Try
                        SamplesOut(C, I) = CType(Reader.ReadInt16(), Double) / 32767.0
                    Catch E As IO.EndOfStreamException
                        Return I
                    End Try
                Next
            Next
            Return MaxSamples
        End Function
    End Class

    REM --------------------------------------------------------------------------
    REM AudioFileWriter Class
    REM --------------------------------------------------------------------------
    Class AudioFileWriter
        Implements IDisposable
        Private Writer As IO.BinaryWriter

        Public Sub New(FileName As String)
            Writer = New IO.BinaryWriter(IO.File.OpenWrite(FileName))
        End Sub

        Protected Sub Dispose() Implements IDisposable.Dispose
            If Writer IsNot Nothing Then
                Writer.Flush()
                Writer.Dispose()
            End If
        End Sub

        Public Sub Write(SamplesOut(,) As Double, SampleCount As Long)
            Dim Channels As Integer = SamplesOut.GetLength(0)
            For I As Long = 0 To SampleCount - 1
                For C As Long = 0 To Channels - 1
                    Writer.Write(CType(SamplesOut(C, I) * 32767.0, Short))
                Next
            Next
        End Sub
    End Class

    REM --------------------------------------------------------------------------
    REM Logging Callback
    REM --------------------------------------------------------------------------
    Private Sub LoggingHandler(Level As Int32, Message As String)
        Select Case Level
            Case 0
                Console.WriteLine("[DynAudNorm][DBG] " & Message)
            Case 1
                Console.WriteLine("[DynAudNorm][WRN] " & Message)
            Case 2
                Console.WriteLine("[DynAudNorm][ERR] " & Message)
            Case Else
                Console.WriteLine("[DynAudNorm][???] " & Message)
        End Select
    End Sub

    REM --------------------------------------------------------------------------
    REM Main Function
    REM --------------------------------------------------------------------------
    Sub Main()
        Dim Version As String
        Version = Assembly.GetExecutingAssembly().GetName().Version.ToString()
        Console.WriteLine("DynamicAudioNormalizer.NET VisualBasic Example [" & Version & "]" & vbCrLf)

        Try
            DynamicAudioNormalizerNET.setLogger(AddressOf LoggingHandler)

            Dim MajorVer As UInteger = 0, MinorVer As UInteger = 0, PatchVer As UInteger = 0
            DynamicAudioNormalizerNET.getVersionInfo(MajorVer, MinorVer, PatchVer)
            Console.WriteLine("Library Version: " & MajorVer & "." & Format(MinorVer, "##00") & "-" & PatchVer)

            Dim DateStr As String = "", TimeStr As String = "", CompilerStr As String = "", ArchStr As String = "", IsDebug As Boolean = False
            DynamicAudioNormalizerNET.getBuildInfo(DateStr, TimeStr, CompilerStr, ArchStr, IsDebug)
            Console.WriteLine("Library Build Date: " & DateStr)
            Console.WriteLine("Library Build Time: " & TimeStr)
            Console.WriteLine("Library Compiler: " & CompilerStr & " (" & ArchStr & ")")
            Console.WriteLine("Library Is Debug: " & IsDebug)

            Using Reader = New AudioFileReader("Input.pcm")
                Using Writer = New AudioFileWriter("Output.pcm")
                    Using Instance = New DynamicAudioNormalizerNET(2, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, True, False, False)
                        ProcessAudioFile(Instance, Reader, Writer)
                    End Using
                End Using
            End Using

            Console.WriteLine("Test completed." & vbCrLf)
        Catch Except As Exception
            Console.WriteLine(vbCrLf & "FAILURE !!!")
            Console.WriteLine(vbCrLf & Except.GetType().ToString() & ": " & Except.Message & vbCrLf)
        End Try
    End Sub

    REM --------------------------------------------------------------------------
    REM Processing Loop
    REM --------------------------------------------------------------------------
    Private Sub ProcessAudioFile(Instance As DynamicAudioNormalizerNET, Reader As AudioFileReader, Writer As AudioFileWriter)
        Dim Channels As UInteger = 0, FilterSize As UInteger = 0, FrameLen As UInteger = 0, SampleRate As UInteger = 0
        Instance.getConfiguration(Channels, FilterSize, FrameLen, SampleRate)
        Console.WriteLine("Channels: " & Channels)
        Console.WriteLine("FilterSize: " & FilterSize)
        Console.WriteLine("FrameLen: " & FrameLen)
        Console.WriteLine("SampleRate: " & SampleRate)

        Dim InternalDelay As Long = Instance.getInternalDelay()
        Console.WriteLine("InternalDelay: " & InternalDelay)

        REM Note: VisualBasic specifies the highest index value (i.e. Length - 1) rather than the Length, so the following creates a 2 x 4096 array!
        Dim AudioSamples = New Double(Channels - 1, 4095) {}

        Console.WriteLine("Processing audio samples, please wait...")
        While True
            Dim InputSamples = Reader.Read(AudioSamples)
            If InputSamples > 0 Then
                Dim OutputSamples = Instance.processInplace(AudioSamples, InputSamples)
                If OutputSamples > 0 Then
                    Writer.Write(AudioSamples, OutputSamples)
                End If
            End If
            If InputSamples < AudioSamples.GetLength(1) Then
                REM End of file reached!
                Exit While
            End If
        End While

        Console.WriteLine("Flushing buffer, please wait...")
        While True
            Dim OutputSamples = Instance.flushBuffer(AudioSamples)
            If OutputSamples > 0 Then
                Writer.Write(AudioSamples, OutputSamples)
                Continue While
            End If
            REM No more samples to flush!
            Exit While
        End While

        REM Only for testing, not usually required!
        Instance.reset()
    End Sub

End Module
