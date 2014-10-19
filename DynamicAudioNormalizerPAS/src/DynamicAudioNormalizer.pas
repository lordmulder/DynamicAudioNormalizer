unit DynamicAudioNormalizer;

interface

uses
  SysUtils, Dialogs;

type
  TDynamicAudioNormalizer = class(TObject)
  public
    constructor Create(const channels: LongWord; const sampleRate: LongWord; const frameLenMsec: LongWord; const filterSize: LongWord; const peakValue: Double; const maxAmplification: Double; const targetRms: Double; const compressFactor: Double; const channelsCoupled: Boolean; const enableDCCorrection: Boolean; const altBoundaryMode: Boolean);
    destructor Destroy; override;
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

function DynAudNorm_CreateInstance(const channels: LongWord; const sampleRate: LongWord; const frameLenMsec: LongWord; const filterSize: LongWord; const peakValue: Double; const maxAmplification: Double; const targetRms: Double; const compressFactor: Double; const channelsCoupled: LongBool; const enableDCCorrection: LongBool; const altBoundaryMode: LongBool; const logFile: Pointer): Pointer; cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'createInstance' + DynamicAudioNormalizerTag);
procedure DynAudNorm_DestroyInstance(var handle: Pointer); cdecl; external DynamicAudioNormalizerDLL name (DynamicAudioNormalizerPre + 'destroyInstance' + DynamicAudioNormalizerTag);

{
function  processInplace(MDynamicAudioNormalizer_Handle *handle, double **samplesInOut, int64_t inputSize, int64_t *outputSize): LongBool;
function  flushBuffer(MDynamicAudioNormalizer_Handle *handle, double **samplesOut, const int64_t bufferSize, int64_t *outputSize): LongBool;
function  reset(MDynamicAudioNormalizer_Handle *handle): LongBool;
function  getConfiguration(MDynamicAudioNormalizer_Handle *handle, LongWord *channels, LongWord *sampleRate, LongWord *frameLen, LongWord *filterSize): LongBool;
function  getInternalDelay(MDynamicAudioNormalizer_Handle *handle, int64_t *delayInSamples): LongBool;
procedure getVersionInfo(LongWord *major, LongWord *minor,LongWord *patch);
procedure getBuildInfo(const char **date, const char **time, const char **compiler, const char **arch, int *debug);
function  setLogFunction(LogFunction) *const logFunction): Pointer;
}

//-----------------------------------------------------------------------------
// Constructor & Destructor
//-----------------------------------------------------------------------------

constructor TDynamicAudioNormalizer.create(const channels: LongWord; const sampleRate: LongWord; const frameLenMsec: LongWord; const filterSize: LongWord; const peakValue: Double; const maxAmplification: Double; const targetRms: Double; const compressFactor: Double; const channelsCoupled: Boolean; const enableDCCorrection: Boolean; const altBoundaryMode: Boolean);
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
  inherited; //Parent Destructor
end;

end.
