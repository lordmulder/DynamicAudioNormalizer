program DynamicAudioNormalizerPAS;

uses
  Forms,
  Main in 'src\Main.pas' {DynamicAudioNormalizerTestApp},
  DynamicAudioNormalizer in 'src\DynamicAudioNormalizer.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.Title := 'Dynamic Audio Normalizer';
  Application.CreateForm(TDynamicAudioNormalizerTestApp, DynamicAudioNormalizerTestApp);
  Application.Run;
end.
