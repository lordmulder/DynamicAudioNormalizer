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

unit Main;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, AppEvnts, Math, ComCtrls;

type
  TDynamicAudioNormalizerTestApp = class(TForm)
    ButtonTest1: TButton;
    ButtonExit: TButton;
    ButtonTest2: TButton;
    ApplicationEvents1: TApplicationEvents;
    ProgressBar1: TProgressBar;
    procedure ButtonTest1Click(Sender: TObject);
    procedure ButtonExitClick(Sender: TObject);
    procedure ButtonTest2Click(Sender: TObject);
    procedure ApplicationEvents1Exception(Sender: TObject; E: Exception);
    procedure FormCreate(Sender: TObject);
  private
    procedure StartStopTest(const start: boolean);
    procedure UpdateProgress;
  public
    { Public-Deklarationen }
  end;

var
  DynamicAudioNormalizerTestApp: TDynamicAudioNormalizerTestApp;

implementation

uses
  DynamicAudioNormalizer;

{$R *.dfm}

//-----------------------------------------------------------------------------
// Log Callback
//-----------------------------------------------------------------------------

{WARNING: This function must use 'cdecl' calling convention!}

procedure loggingHandler(const level: LongInt; const text: PAnsiChar); cdecl;
var
  tag: String;
begin
  case level of
    0: tag := 'DBG';
    1: tag := 'WRN';
    2: tag := 'ERR';
  else
    tag := '???';
  end;

  WriteLn(Format('[DynAudNorm][%s] %s', [tag, text]));
end;

//-----------------------------------------------------------------------------
// Helper Functions
//-----------------------------------------------------------------------------

procedure TDynamicAudioNormalizerTestApp.UpdateProgress;
begin
  if ProgressBar1.Position < ProgressBar1.Max then
  begin
    ProgressBar1.StepIt;
  end else
  begin
    ProgressBar1.Position := 0;
  end;
  Application.ProcessMessages;
end;

procedure TDynamicAudioNormalizerTestApp.StartStopTest(const start: boolean);
var
  i: Integer;
begin
  if start then
  begin
    for i := 0 to Self.ControlCount-1 do
    begin
      Self.Controls[i].Enabled := False;
    end;
    ProgressBar1.Position := 0;
  end else
  begin
    MessageBeep(1000);
    for i := 0 to Self.ControlCount-1 do
    begin
      Self.Controls[i].Enabled := True;
    end;
    ProgressBar1.Position := ProgressBar1.Max;
  end;
end;

//-----------------------------------------------------------------------------
// Misc Functions
//-----------------------------------------------------------------------------

procedure TDynamicAudioNormalizerTestApp.FormCreate(Sender: TObject);
begin
  TDynamicAudioNormalizer.SetLogFunction(@loggingHandler);
end;

procedure TDynamicAudioNormalizerTestApp.ButtonExitClick(Sender: TObject);
begin
  Application.Terminate;
end;

procedure TDynamicAudioNormalizerTestApp.ApplicationEvents1Exception(Sender: TObject; E: Exception);
begin
  MessageBox(self.Handle, @E.Message[1], 'ERROR', MB_ICONERROR or MB_TOPMOST);
  ExitProcess(DWORD(-1));
end;

//-----------------------------------------------------------------------------
// TEST #1
//-----------------------------------------------------------------------------

procedure TDynamicAudioNormalizerTestApp.ButtonTest1Click(Sender: TObject);
const
  rounds = 64;
  instances = 16;
var
  normalizer: Array [1..instances] of TDynamicAudioNormalizer;
  i,k: Cardinal;
begin
  StartStopTest(True);

  for k := 1 to rounds do
  begin
    WriteLn('Round ' + IntToStr(k) + ' of ' + IntToStr(rounds) + '.');
    for i := 1 to instances do
    begin
      normalizer[i] := TDynamicAudioNormalizer.Create(2, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, True, False, False);
      if i mod 4 = 0 then UpdateProgress;
    end;
    for i := 1 to instances do
    begin
      FreeAndNil(normalizer[i]);
      if i mod 4 = 0 then UpdateProgress;
    end;
  end;
  
  WriteLn('Done.'#10);
  StartStopTest(False);
end;

//-----------------------------------------------------------------------------
// TEST #2
//-----------------------------------------------------------------------------

procedure TDynamicAudioNormalizerTestApp.ButtonTest2Click(Sender: TObject);
const
  frameSize = 4096;
  channelCount = 2;
var
  samples: Array of TDoubleArray;
  buffer: array[0..((channelCount*frameSize)-1)] of SmallInt;
  inputFile, outputFile: THandle;
  normalizer: TDynamicAudioNormalizer;
  i, j, k, q: Cardinal;
  readSize, writeSize, bytesWritten: Cardinal;
begin
  StartStopTest(True);

  {============================ SETUP ============================}

  SetLength(samples, channelCount, frameSize);
  normalizer := TDynamicAudioNormalizer.Create(channelCount, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, True, False, False);

  WriteLn('Open I/O files...');
  inputFile  := CreateFile('Input.pcm',  GENERIC_READ, FILE_SHARE_READ, nil, OPEN_EXISTING, 0, 0);
  outputFile := CreateFile('Output.pcm', GENERIC_WRITE, 0, nil, CREATE_ALWAYS, 0, 0);

  if (inputFile = INVALID_HANDLE_VALUE) or (outputFile = INVALID_HANDLE_VALUE) then
  begin
    Raise Exception.Create('Failed to open input/output files!');
  end;

  {=========================== PROCESS ===========================}

  WriteLn('Processing, please wait...');
  for q := 0 to 2147483647 do
  begin
    if not ReadFile(inputFile, buffer, frameSize * channelCount * SizeOf(SmallInt), readSize, nil) then
    begin
      Raise Exception.Create('Read operation has failed!');
    end;

    readSize := readSize div (channelCount * SizeOf(SmallInt));

    if readSize > 0 then
    begin
      k := 0;
      for i := 0 to (readSize-1) do
      begin
        for j := 0 to (channelCount-1) do
        begin
          samples[j][i] := buffer[k] / 32767.0;
          k := k + 1;
        end;
      end;
    end;

    writeSize := normalizer.ProcessInplace(samples, readSize);
    //WriteLn('Result: readSize=' + IntToStr(readSize) + '; writeSize=' +  IntToStr(writeSize));

    if writeSize > 0 then
    begin
      k := 0;
      for i := 0 to (writeSize-1) do
      begin
        for j := 0 to (channelCount-1) do
        begin
          buffer[k] := SmallInt(Round(Max(-1.0,Min(1.0,samples[j][i])) * 32767.0));
          k := k + 1;
        end;
      end;
      if not WriteFile(outputFile, buffer, writeSize * channelCount * SizeOf(SmallInt), bytesWritten, nil) then
      begin
        Raise Exception.Create('Write operation has failed!');
      end;
    end;

    if readSize < frameSize then
    begin
      Break; {End of File}
    end;

    if q mod 64 = 0 then UpdateProgress;
  end;

  {============================ FLUSH ============================}

  WriteLn('Flushing buffer, please wait...');
  for q := 0 to 2147483647 do
  begin
    writeSize := normalizer.FlushBuffer(samples);

    if writeSize > 0 then
    begin
      k := 0;
      for i := 0 to (writeSize-1) do
      begin
        for j := 0 to (channelCount-1) do
        begin
          buffer[k] := SmallInt(Round(Max(-1.0,Min(1.0,samples[j][i])) * 32767.0));
          k := k + 1;
        end;
      end;
      if not WriteFile(outputFile, buffer, writeSize * channelCount * SizeOf(SmallInt), bytesWritten, nil) then
      begin
        Raise Exception.Create('Write operation has failed!');
      end;
    end;

    if writeSize < 1 then
    begin
      Break; {No more samples left}
    end;

    if q mod 16 = 0 then UpdateProgress;
  end;

  {============================ CLOSE ============================}

  FreeAndNil(normalizer);
  FlushFileBuffers(outputFile);

  CloseHandle(inputFile);
  CloseHandle(outputFile);

  WriteLn('Done.'#10);
  StartStopTest(False);
end;

end.
