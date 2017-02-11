object DynamicAudioNormalizerTestApp: TDynamicAudioNormalizerTestApp
  Left = 618
  Top = 301
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'DynAudNorm Testbed'
  ClientHeight = 145
  ClientWidth = 267
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnCreate = FormCreate
  PixelsPerInch = 96
  TextHeight = 13
  object ButtonTest1: TButton
    Left = 8
    Top = 8
    Width = 249
    Height = 25
    Caption = 'Create and Destory many instances'
    TabOrder = 0
    OnClick = ButtonTest1Click
  end
  object ButtonExit: TButton
    Left = 8
    Top = 112
    Width = 249
    Height = 25
    Caption = 'Exit'
    TabOrder = 1
    OnClick = ButtonExitClick
  end
  object ButtonTest2: TButton
    Left = 8
    Top = 40
    Width = 249
    Height = 25
    Caption = 'Process complete Audio File'
    TabOrder = 2
    OnClick = ButtonTest2Click
  end
  object ProgressBar1: TProgressBar
    Left = 8
    Top = 80
    Width = 249
    Height = 17
    Smooth = True
    TabOrder = 3
  end
  object ApplicationEvents1: TApplicationEvents
    OnException = ApplicationEvents1Exception
    Left = 8
    Top = 72
  end
  object XPManifest1: TXPManifest
    Left = 40
    Top = 72
  end
  object OpenDialog: TOpenDialog
    DefaultExt = 'pcm'
    Filter = 'Raw PCM data (*.pcm)|*.pcm'
    Options = [ofHideReadOnly, ofNoChangeDir, ofPathMustExist, ofFileMustExist, ofEnableSizing]
    Title = 'Select Source File'
    Left = 72
    Top = 72
  end
  object SaveDialog: TSaveDialog
    DefaultExt = 'pcm'
    Filter = 'Raw PCM data (*.pcm)|*.pcm'
    Options = [ofOverwritePrompt, ofHideReadOnly, ofNoChangeDir, ofPathMustExist, ofEnableSizing]
    Title = 'Select Output File'
    Left = 104
    Top = 72
  end
end
