//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Delphi/Pascal Wrapper
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

// *******************************************************************************
// NOTE: This code was developed and tested with Borland Delphi 7.1 Professional
// *******************************************************************************

unit DynamicAudioNormalizer;

//=============================================================================
interface
//=============================================================================

uses
  SysUtils;

type
  TDoubleArray = array of Double;

type
  TLoggingFunction = procedure(const level: LongInt; const text: PAnsiChar); cdecl;
  PLoggingFunction = ^TLoggingFunction;

type
  TDynamicAudioNormalizer = class(TObject)
  public
    //Constructor & Destructor
    constructor Create(const channels: LongWord; const sampleRate: LongWord; const frameLenMsec: LongWord; const filterSize: LongWord; const peakValue: Double; const maxAmplification: Double; const targetRms: Double; const compressFactor: Double; const channelsCoupled: Boolean; const enableDCCorrection: Boolean; const altBoundaryMode: Boolean);
    destructor Destroy; override;
    //Processing Functions
    function Process(samplesIn: Array of TDoubleArray; samplesOut: Array of TDoubleArray; const sampleCount: Int64): Int64;
    function ProcessInplace(samplesInOut: Array of TDoubleArray; const sampleCount: Int64): Int64;
    function FlushBuffer(samplesOut: Array of TDoubleArray): Int64;
    procedure Reset;
    //Utility Functions
    procedure GetConfiguration(var channels: LongWord; var sampleRate: LongWord; var frameLen: LongWord; var filterSize: LongWord);
    function GetInternalDelay: Int64;
    //Static Functions
    class function SetLogFunction(const logFunction: PLoggingFunction): PLoggingFunction;
    class procedure GetVersionInfo(var major: LongWord; var minor: LongWord; var patch: LongWord);
    class procedure GetBuildInfo(var date: PAnsiChar; var time: PAnsiChar; var compiler: PAnsiChar; var arch: PAnsiChar; var debug: LongBool);
  private
    instance: Pointer;
  end;

//=============================================================================
implementation
//=============================================================================

const DynamicAudioNormalizerTag = '_r8';
const DynamicAudioNormalizerDLL = 'DynamicAudioNormalizerAPI.dll';
const DynamicAudioNormalizerPre = 'MDynamicAudioNormalizer_';

//-----------------------------------------------------------------------------
// Native Functions
//-----------------------------------------------------------------------------

function  DynAudNorm_CreateInstance(const channels: LongWord; const sampleRate: LongWord; const frameLenMsec: LongWord; const filterSize: LongWord; const peakValue: Double; const maxAmplification: Double; const targetRms: Double; const compressFactor: Double; const channelsCoupled: LongBool; const enableDCCorrection: LongBool; const altBoundaryMode: LongBool; const logFile: Pointer): Pointer; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'createInstance' + DynamicAudioNormalizerTag);
procedure DynAudNorm_DestroyInstance(var handle: Pointer); cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'destroyInstance' + DynamicAudioNormalizerTag);
function  DynAudNorm_Process(const handle: Pointer; const samplesIn: Pointer; const samplesOut: Pointer; const inputSize: Int64; var outputSize: Int64): LongBool; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'processInplace' + DynamicAudioNormalizerTag);
function  DynAudNorm_ProcessInplace(const handle: Pointer; const samplesInOut: Pointer; const inputSize: Int64; var outputSize: Int64): LongBool; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'processInplace' + DynamicAudioNormalizerTag);
function  DynAudNorm_FlushBuffer(const handle: Pointer; const samplesOut: Pointer; const bufferSize: Int64; var outputSize: Int64): LongBool; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'flushBuffer' + DynamicAudioNormalizerTag);
function  DynAudNorm_GetConfiguration(const handle: Pointer; var channels: LongWord; var sampleRate: LongWord; var frameLen: LongWord; var filterSize: LongWord): LongBool; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'getConfiguration' + DynamicAudioNormalizerTag);
function  DynAudNorm_Reset(const handle: Pointer): LongBool; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'reset' + DynamicAudioNormalizerTag);
function  DynAudNorm_GetInternalDelay(const handle: Pointer; var delayInSamples: Int64): LongBool; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'getInternalDelay' + DynamicAudioNormalizerTag);
function  DynAudNorm_SetLogFunction(const logFunction: PLoggingFunction): PLoggingFunction; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'setLogFunction' + DynamicAudioNormalizerTag);
function  DynAudNorm_GetVersionInfo(var major: LongWord; var minor: LongWord; var patch: LongWord): LongBool; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'getVersionInfo' + DynamicAudioNormalizerTag);
function  DynAudNorm_GetBuildInfo(var date: PAnsiChar; var time: PAnsiChar; var compiler: PAnsiChar; var arch: PAnsiChar; var debug: LongBool): LongBool; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'getBuildInfo' + DynamicAudioNormalizerTag);

//-----------------------------------------------------------------------------
// Static Functions
//-----------------------------------------------------------------------------

class procedure TDynamicAudioNormalizer.GetVersionInfo(var major: LongWord; var minor: LongWord; var patch: LongWord);
begin
  if not DynAudNorm_GetVersionInfo(major, minor, patch) then
  begin
    Raise Exception.Create('Failed to retrieve library version info!');
  end;
end;

class procedure TDynamicAudioNormalizer.GetBuildInfo(var date: PAnsiChar; var time: PAnsiChar; var compiler: PAnsiChar; var arch: PAnsiChar; var debug: LongBool);
begin
  if not DynAudNorm_GetBuildInfo(date, time, compiler, arch, debug) then
  begin
    Raise Exception.Create('Failed to retrieve library build configuration!');
  end;
end;

class function TDynamicAudioNormalizer.SetLogFunction(const logFunction: PLoggingFunction): PLoggingFunction;
begin
  Result := DynAudNorm_SetLogFunction(logFunction);
end;

//-----------------------------------------------------------------------------
// Constructor & Destructor
//-----------------------------------------------------------------------------

constructor TDynamicAudioNormalizer.Create(const channels: LongWord; const sampleRate: LongWord; const frameLenMsec: LongWord; const filterSize: LongWord; const peakValue: Double; const maxAmplification: Double; const targetRms: Double; const compressFactor: Double; const channelsCoupled: Boolean; const enableDCCorrection: Boolean; const altBoundaryMode: Boolean);
begin
  instance := DynAudNorm_CreateInstance(channels, sampleRate, frameLenMsec, filterSize, peakValue, maxAmplification, targetRms, compressFactor, channelsCoupled, enableDCCorrection, altBoundaryMode, nil);
  if not Assigned(instance) then
  begin
    Raise Exception.Create('Failed to create DynamicAudioNormalizer instance!');
  end;
end;

destructor TDynamicAudioNormalizer.Destroy;
begin
  if Assigned(instance) then
  begin
    DynAudNorm_DestroyInstance(instance);
    if Assigned(instance) then
    begin
      Raise Exception.Create('Failed to destroy DynamicAudioNormalizer instance!');
    end;
  end;
  inherited; {Parent Destructor}
end;

//-----------------------------------------------------------------------------
// Processing Functions
//-----------------------------------------------------------------------------

function TDynamicAudioNormalizer.Process(samplesIn: Array of TDoubleArray; samplesOut: Array of TDoubleArray; const sampleCount: Int64): Int64;
var
  bufferIn, bufferOut: Array of Pointer;
  i: LongWord;
  channels, sampleRate, frameLen, filterSize: LongWord;
  outputSize: Int64;
begin
  if not Assigned(instance) then
  begin
    Raise Exception.Create('Native instance not created yet!');
  end;

  GetConfiguration(channels, sampleRate, frameLen, filterSize);
  if Length(samplesIn) <> Integer(channels) then
  begin
    Raise Exception.Create('Array dimension doesn''t match channel count!');
  end;
  if Length(samplesOut) <> Integer(channels) then
  begin
    Raise Exception.Create('Array dimension doesn''t match channel count!');
  end;

  SetLength(bufferIn,  Length(samplesIn));
  SetLength(bufferOut, Length(samplesOut));

  for i := 0 to Length(samplesIn)-1 do
  begin
    if Length(samplesIn[i]) < sampleCount then
    begin
      Raise Exception.Create('Array length is smaller than specified sample count!');
    end;
    if Length(samplesOut[i]) < sampleCount then
    begin
      Raise Exception.Create('Array length is smaller than specified sample count!');
    end;
    bufferIn[i]  := @samplesIn [i][0];
    bufferOut[i] := @samplesOut[i][0];
  end;

  if not DynAudNorm_Process(instance, @bufferIn[0], @bufferOut[0], sampleCount, outputSize) then
  begin
    Raise Exception.Create('Failed to process samples in-place!');
  end;

  Result := outputSize;
end;

function TDynamicAudioNormalizer.ProcessInplace(samplesInOut: Array of TDoubleArray; const sampleCount: Int64): Int64;
var
  buffer: Array of Pointer;
  i: LongWord;
  channels, sampleRate, frameLen, filterSize: LongWord;
  outputSize: Int64;
begin
  if not Assigned(instance) then
  begin
    Raise Exception.Create('Native instance not created yet!');
  end;

  GetConfiguration(channels, sampleRate, frameLen, filterSize);
  if Length(samplesInOut) <> Integer(channels) then
  begin
    Raise Exception.Create('Array dimension doesn''t match channel count!');
  end;

  SetLength(buffer, Length(samplesInOut));

  for i := 0 to Length(samplesInOut)-1 do
  begin
    if Length(samplesInOut[i]) < sampleCount then
    begin
      Raise Exception.Create('Array length is smaller than specified sample count!');
    end;
    buffer[i] := @samplesInOut[i][0];
  end;

  if not DynAudNorm_ProcessInplace(instance, @buffer[0], sampleCount, outputSize) then
  begin
    Raise Exception.Create('Failed to process samples in-place!');
  end;

  Result := outputSize;
end;

function TDynamicAudioNormalizer.FlushBuffer(samplesOut: Array of TDoubleArray): Int64;
var
  buffer: Array of Pointer;
  i: LongWord;
  channels, sampleRate, frameLen, filterSize: LongWord;
  outputSize: Int64;
begin
  if not Assigned(instance) then
  begin
    Raise Exception.Create('Native instance not created yet!');
  end;

  GetConfiguration(channels, sampleRate, frameLen, filterSize);
  if Length(samplesOut) <> Integer(channels) then
  begin
    Raise Exception.Create('Array dimension doesn''t match channel count!');
  end;
  
  SetLength(buffer, Length(samplesOut));

  for i := 0 to Length(samplesOut)-1 do
  begin
    if Length(samplesOut[i]) <> Length(samplesOut[0]) then
    begin
      Raise Exception.Create('Lengths of "inner" array''s are *not* inconsistent!');
    end;
    buffer[i] := @samplesOut[i][0];
  end;

  if not DynAudNorm_FlushBuffer(instance, @buffer[0], Length(samplesOut[0]), outputSize) then
  begin
    Raise Exception.Create('Failed to flush samples from buffer!');
  end;

  Result := outputSize;
end;

procedure  TDynamicAudioNormalizer.Reset;
begin
  if not Assigned(instance) then
  begin
    Raise Exception.Create('Native instance not created yet!');
  end;

  if not DynAudNorm_Reset(instance) then
  begin
    Raise Exception.Create('Failed to reset DynamicAudioNormalizer instance!');
  end;
end;

//-----------------------------------------------------------------------------
// Utility Functions
//-----------------------------------------------------------------------------

procedure TDynamicAudioNormalizer.GetConfiguration(var channels: LongWord; var sampleRate: LongWord; var frameLen: LongWord; var filterSize: LongWord);
begin
  if not Assigned(instance) then
  begin
    Raise Exception.Create('Native instance not created yet!');
  end;

  if not DynAudNorm_GetConfiguration(instance, channels, sampleRate, frameLen, filterSize) then
  begin
    Raise Exception.Create('Failed to get current configuration!');
  end;
end;

function TDynamicAudioNormalizer.GetInternalDelay: Int64;
begin
  if not Assigned(instance) then
  begin
    Raise Exception.Create('Native instance not created yet!');
  end;

  if not DynAudNorm_GetInternalDelay(instance, Result) then
  begin
    Raise Exception.Create('Failed to get current configuration!');
  end;
end;

end.
