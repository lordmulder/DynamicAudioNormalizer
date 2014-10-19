unit Main;

interface

uses
  Windows, Messages, SysUtils, Variants, Classes, Graphics, Controls, Forms,
  Dialogs, StdCtrls;

type
  TDynamicAudioNormalizerTestApp = class(TForm)
    Button1: TButton;
    procedure Button1Click(Sender: TObject);
  private
    { Private-Deklarationen }
  public
    { Public-Deklarationen }
  end;

var
  DynamicAudioNormalizerTestApp: TDynamicAudioNormalizerTestApp;

implementation

uses
  DynamicAudioNormalizer;

{$R *.dfm}

procedure TDynamicAudioNormalizerTestApp.Button1Click(Sender: TObject);
var
  normalizer: Array [0..7] of TDynamicAudioNormalizer;
  i: Cardinal;
begin
  for i := 0 to 7 do
  begin
    normalizer[i] := TDynamicAudioNormalizer.Create(2, 44100, 500, 31, 0.95, 10.0, 0.0, 0.0, True, False, False);
  end;

  for i := 0 to 7 do
  begin
    FreeAndNil(normalizer[i]);
  end;
end;

end.
