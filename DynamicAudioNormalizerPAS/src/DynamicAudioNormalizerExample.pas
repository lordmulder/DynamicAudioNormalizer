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

unit DynamicAudioNormalizerExample;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls, AppEvnts, Math, DateUtils, ComCtrls, XPMan;

type
  TDynamicAudioNormalizerTestApp = class(TForm)
    ButtonTest1: TButton;
    ButtonExit: TButton;
    ButtonTest2: TButton;
    ApplicationEvents1: TApplicationEvents;
    ProgressBar1: TProgressBar;
    XPManifest1: TXPManifest;
    OpenDialog: TOpenDialog;
    SaveDialog: TSaveDialog;
    procedure ButtonTest1Click(Sender: TObject);
    procedure ButtonExitClick(Sender: TObject);
    procedure ButtonTest2Click(Sender: TObject);
    procedure ApplicationEvents1Exception(Sender: TObject; E: Exception);
    procedure FormCreate(Sender: TObject);
  private
    LastUpdate: TDateTime;
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
    ProgressBar1.StepBy(1);
    LastUpdate := Now;
  end else
  begin
    if MilliSecondsBetween(Now, LastUpdate) >= 997 then
    begin
      ProgressBar1.Position := 0;
      LastUpdate := Now;
    end
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
var
  major, minor, patch: LongWord;
  date, time, compiler, arch: PAnsiChar;
  debug: LongBool;
begin
  LastUpdate := Now;

  TDynamicAudioNormalizer.GetVersionInfo(major, minor, patch);
  WriteLn(Format('Library Version: %s', [StringReplace(Format('%u.%02u-%u', [major, minor, patch]), ' ', '0', [rfReplaceAll])]));

  TDynamicAudioNormalizer.GetBuildInfo(date, time, compiler, arch, debug);
  WriteLn(Format('Build: Date=%s; Time=%s; Compiler=%s; Arch=%s; Debug=%s'#10, [date, time, compiler, arch, BoolToStr(debug)]));

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
  rounds = 97;
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
  
  MessageBox(self.WindowHandle, 'Test completed successfully.', PAnsiChar(self.Caption), MB_ICONINFORMATION);
end;

//-----------------------------------------------------------------------------
// TEST #2
//-----------------------------------------------------------------------------

procedure TDynamicAudioNormalizerTestApp.ButtonTest2Click(Sender: TObject);
const
  frameSize = 4096;
  channelCount = 2;
  MAX_SHORT: Double = 32767.0;
var
  samples: Array of TDoubleArray;
  buffer: array[0..((channelCount*frameSize)-1)] of SmallInt;
  inputFile, outputFile: THandle;
  normalizer: TDynamicAudioNormalizer;
  i, j, k, q: Cardinal;
  readSize, writeSize, bytesWritten: Cardinal;
begin
  {========================= SELECT FILES ========================}

  if not OpenDialog.Execute then
  begin
    Exit;
  end;

  if not SaveDialog.Execute then
  begin
    Exit;
  end;

  {============================ SETUP ============================}

  StartStopTest(True);
  
  SetLength(samples, channelCount, frameSize);

  WriteLn('Open I/O files...');
  inputFile  := CreateFile(PAnsiChar(OpenDialog.Files[0]),  GENERIC_READ, FILE_SHARE_READ, nil, OPEN_EXISTING, 0, 0);
  outputFile := CreateFile(PAnsiChar(SaveDialog.Files[0]), GENERIC_WRITE, 0, nil, CREATE_ALWAYS, 0, 0);

  if (inputFile = INVALID_HANDLE_VALUE) or (outputFile = INVALID_HANDLE_VALUE) then
  begin
    Raise Exception.Create('Failed to open input/output files!');
  end;

  WriteLn('Creating normalizer instance...');
  normalizer := TDynamicAudioNormalizer.Create(channelCount, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, True, False, False);
  WriteLn('Internal delay: ' + IntToStr(normalizer.GetInternalDelay));

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
          samples[j][i] := buffer[k] / MAX_SHORT;
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
          buffer[k] := SmallInt(Round(Max(-1.0,Min(1.0,samples[j][i])) * MAX_SHORT));
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

    if q mod 32 = 0 then
    begin
      UpdateProgress;
    end;
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
          buffer[k] := SmallInt(Round(Max(-1.0,Min(1.0,samples[j][i])) * MAX_SHORT));
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

    if q mod 32 = 0 then
    begin
      UpdateProgress;
    end;
  end;

  {============================ CLOSE ============================}

  WriteLn('Shutting down...');
  normalizer.Reset(); {Just to make sure that the Reset() function works}

  {============================ RESET ============================}

  FreeAndNil(normalizer);
  FlushFileBuffers(outputFile);

  CloseHandle(inputFile);
  CloseHandle(outputFile);

  WriteLn('Done.'#10);
  StartStopTest(False);

  MessageBox(self.WindowHandle, 'Test completed successfully.', PAnsiChar(self.Caption), MB_ICONINFORMATION);
end;

end.
