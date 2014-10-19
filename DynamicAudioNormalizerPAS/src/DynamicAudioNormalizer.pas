//////////////////////////////////////////////////////////////////////////////////
// Dynamic Audio Normalizer - Delphi/Pascal Wrapper
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

unit DynamicAudioNormalizer;

interface

uses
  SysUtils, Dialogs;

type
  TDoubleArray = array of Double;

type
  TLoggingFunction = procedure(const level: LongInt; const text: PAnsiChar);
  PLoggingFunction = ^TLoggingFunction;

type
  TDynamicAudioNormalizer = class(TObject)
  public
    constructor Create(const channels: LongWord; const sampleRate: LongWord; const frameLenMsec: LongWord; const filterSize: LongWord; const peakValue: Double; const maxAmplification: Double; const targetRms: Double; const compressFactor: Double; const channelsCoupled: Boolean; const enableDCCorrection: Boolean; const altBoundaryMode: Boolean);
    destructor Destroy; override;

    function ProcessInplace(samplesInOut: Array of TDoubleArray; const sampleCount: Int64): Int64;
    function FlushBuffer(samplesOut: Array of TDoubleArray): Int64;

    procedure GetConfiguration(var channels: LongWord; var sampleRate: LongWord; var frameLen: LongWord; var filterSize: LongWord);
  private
    instance: Pointer;
  end;

implementation

//-----------------------------------------------------------------------------
// Native Functions
//-----------------------------------------------------------------------------

const DynamicAudioNormalizerTag = '_r7';
const DynamicAudioNormalizerDLL = 'DynamicAudioNormalizerAPI.dll';
const DynamicAudioNormalizerPre = 'MDynamicAudioNormalizer_';

function  DynAudNorm_CreateInstance(const channels: LongWord; const sampleRate: LongWord; const frameLenMsec: LongWord; const filterSize: LongWord; const peakValue: Double; const maxAmplification: Double; const targetRms: Double; const compressFactor: Double; const channelsCoupled: LongBool; const enableDCCorrection: LongBool; const altBoundaryMode: LongBool; const logFile: Pointer): Pointer; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'createInstance' + DynamicAudioNormalizerTag);
procedure DynAudNorm_DestroyInstance(var handle: Pointer); cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'destroyInstance' + DynamicAudioNormalizerTag);
function  DynAudNorm_ProcessInplace(const handle: Pointer; const samplesInOut: Pointer; const inputSize: Int64; var outputSize: Int64): LongBool; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'processInplace' + DynamicAudioNormalizerTag);
function  DynAudNorm_FlushBuffer(const handle: Pointer; const samplesOut: Pointer; const bufferSize: Int64; var outputSize: Int64): LongBool; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'flushBuffer' + DynamicAudioNormalizerTag);
function  DynAudNorm_GetConfiguration(const handle: Pointer; var channels: LongWord; var sampleRate: LongWord; var frameLen: LongWord; var filterSize: LongWord): LongBool; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'getConfiguration' + DynamicAudioNormalizerTag);

{
function  reset(MDynamicAudioNormalizer_Handle *handle): LongBool;
function  getInternalDelay(MDynamicAudioNormalizer_Handle *handle, int64_t *delayInSamples): LongBool;
procedure getVersionInfo(LongWord *major, LongWord *minor,LongWord *patch);
procedure getBuildInfo(const char **date, const char **time, const char **compiler, const char **arch, int *debug);
function  setLogFunction(LogFunction) *const logFunction): Pointer;
}

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
// ProcessingFunctions
//-----------------------------------------------------------------------------

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

end.
