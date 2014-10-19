object DynamicAudioNormalizerTestApp: TDynamicAudioNormalizerTestApp
  Left = 618
  Top = 301
  BorderIcons = [biSystemMenu, biMinimize]
  BorderStyle = bsSingle
  Caption = 'DynamicAudioNormalizerTestApp'
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
    TabOrder = 3
  end
  object ApplicationEvents1: TApplicationEvents
    OnException = ApplicationEvents1Exception
    Left = 8
    Top = 8
  end
end
