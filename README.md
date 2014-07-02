Dynamic Audio Normalizer
===============================================================================

<small>Created by LoRd_MuldeR <<mulder2@gmx>></tt> – Please check http://muldersoft.com/ for news and updates!</small>

**Dynamic Audio Normalizer** is a library and a command-line tool for [audio normalization](http://en.wikipedia.org/wiki/Audio_normalization). It applies a certain amount of gain to the input audio in order to bring its peak magnitude to a target level (e.g. 0 dBFS). However, in contrast to more "simple" normalization algorithms, the Dynamic Audio Normalizer *dynamically* adjusts the gain factor to the input audio. This allows for applying extra gain to the "quiet" parts of the audio while avoiding distortions or clipping the "loud" parts. In other words, the volume of the "quiet" and the "loud" parts will be harmonized.

-------------------------------------------------------------------------------
How it works
-------------------------------------------------------------------------------

The "standard" audio normalization algorithm applies the same *constant* amount of gain to *all* samples in the file. Consequently, the gain factor must be chosen in a way that won't cause clipping/distortion – even for the input sample that has the highest magnitude. So if <tt>S_max</tt> denotes the highest magnitude sample in the input audio and <tt>Peak</tt> is the desired peak magnitude, then the gain factor will be chosen as <tt>G=Peak/abs(S_max)</tt>. This works fine, as long as the volume of the input audio remains constant, more or less, all the time. If, however, the volume of the input audio varies significantly over time – as is the case with many "real world" recordings – the standard normalization algorithm will *not* give satisfying result. That's because the "loud" parts can *not* be amplified any further (without distortions) and thus the "quiet" parts will remain quiet too.

Dynamic Audio Normalizer solves this problem by processing the input audio in small chunks, referred to as *frames*. A frame typically has a length 500 milliseconds, but the frame size can be adjusted as needed. It then finds the highest magnitude sample *within* each frame. Finally it computes the maximum possible gain factor (without distortions) for each individual frame. So if <tt>S_max[n]</tt> denotes the highest magnitude sample within the <tt>n</tt>-th frame, then the maximum possible gain factor for the <tt>n</tt>-th frame will be <tt>G[n]=Peak/abs(S_max[n])</tt>. Unfortunately, simply amplifying each frame with its own "local" maximum gain factor <tt>G[n]</tt> would *not* give satisfying results either. That's because the maximum gain factors can vary *strongly* and *unsteadily* between neighbouring frames! Therefore, applying the maximum possible gain to each frame *without* taking neighbouring frames into account would essentially result in a strong *dynamic range compression* – which not only has a tendency to destroy the "vividness" of the audio but can also result in the nasty "pumping" effect, i.e fast changes of the gain factor that become clearly noticeable to the listener.

The Dynamic Audio Normalizer tries to avoid these issues by applying an advanced *2-Pass* normalization algorithm. Essentially, while the *first* pass computes (and stores) the maximum "local" gain factor <tt>G[n]</tt> for each individual frame, the actual normalization takes place in the *second* pass. Before the second pass starts, a [*Gaussian* smoothing kernel](http://en.wikipedia.org/wiki/Gaussian_blur) will be applied to the gain factors. Put simply, this filter "mixes" the gain factor of the <tt>n</tt>-th frames with those of all its preceding frames (<tt>n-1</tt>, <tt>n-2</tt>, &hellip;) as well as with all its subsequent frames (<tt>n+1</tt>, <tt>n+2</tt>, …) – where "near by" frames, in both directions, have a stronger influence, while "far away" frames, in both directions, have a declining influence. This way, abrupt changes of the gain factor are avoided and, instead, we get *smooth transitions* of the gain factor over time. Furthermore, since the filter also takes into account *future* frames, Dynamic Audio Normalizer avoids applying strong gain to "quiet" frames located shortly before "loud" frames. This can also be understood as a *lookahead* function which adjusts the gain factor *early* and thus nicely prevents clipping/distortion or abrupt gain reductions.

One more detail to consider is that applying the Gaussian smoothing kernel alone can *not* solve all problems. That's because the smoothing kernel will *not* only smoothen/delay *increasing* gain factors but also *declining* ones! If, for example, a very "loud" frame follows immediately after a sequence of "quiet" frames, the smoothing causes the gain factor to decrease early but slowly. As a result, the *filtered* gain factor of the "loud" frame could actually turn out to be *higher* than its (local) maximum gain factor – which results in distortion/clipping, if not taken care of! For this reason, the Dynamic Audio Normalizer *additionally* applies a "minimum" filter, i.e. a filter that replaces each gain factor with the *smallest* value within a certain neighbourhood. This is done *before* the Gaussian smoothing kernel in order to ensure that all gain transitions will remain smooth.

The following example shows the results form a "real world" audio recording that has been processed by the Dynamic Audio Normalizer. The chart shows the maximum local gain factors for each individual frame (blue) as well as the minimum filtered gain factors (green) and the final smoothend gain factors (orange). Note how smooth the progression of the final gain factors is, while approaching the maximum local gain factors as closely as possible. Also note how the smoothend gain factors *never* exceed the maximum local gain factor in order to avoid distortions.

![Chart](img/Chart.png "Dynamic Audio Normalizer – Example")  
<small>**Figure 1:** Progression of the gain factors for each audio frame.</small>  
<br>

So far it has been discussed how the optimal gain factor for each frame is determined. However, since each frame contains a large number of samples – at a typical sampling rate of 44,100 Hz and a standard frame size of 500 milliseconds we have 22,050 samples per frame – it is also required to infer the gain factor for each individual sample in the frame. The most simple approach, of course, is applying the *same* gain factor to *all* samples in the certain frame. But this would lead to abrupt changes of the gain factor at each frame boundary, while the gain factor remains constant within the frames. A better approach, as implemented in the Dynamic Audio Normalizer, is interpolating the per-sample gain factors. In particular, the Dynamic Audio Normalizer applies a *linear interpolation* in order to compute the gain factors for the samples inside the <tt>n</tt>-th frame from the gain factors <tt>G'[n-1]</tt>, <tt>G'[n]</tt> and <tt>G'[n+1]</tt>, where <tt>G'[k]</tt> denotes the final *filtered* gain factor for the <tt>k</tt>-th frame. The following graph shows how the per-sample gain factors (orange) are interpolated from the gain factors of the preceding (green), current (blue) and subsequent (purple) frame.

![Interpolation](img/Interpolation.png "Dynamic Audio Normalizer – Interpolation")  
<small>**Figure 2:** Linear interpolation of the per-sample gain factors.</small>  
<br>

Finally, the following waveform view illustrates how the volume of a "real world" audio recording has been harmonized by the Dynamic Audio Normalizer. The upper view shows the unprocessed original recording while the lower view shows the output as created by the Dynamic Audio Normalizer. As can be seen, the significant volume variation between the "loud" and the "quiet" parts that existed in the original recording has been rectified to a great extent, while retaining the dynamics of the input and avoiding clipping or distortion.

![Waveform](img/Waveform.png "Dynamic Audio Normalizer – Example")  
<small>**Figure 3:** Waveform before and after processing.</small>

-------------------------------------------------------------------------------
Command-Line Usage
-------------------------------------------------------------------------------

Dynamic Audio Normalizer program can be invoked via [command-line interface](http://en.wikipedia.org/wiki/Command-line_interface), e.g. manually from the [command prompt](http://en.wikipedia.org/wiki/Command_Prompt) or automatically by a script file. The basic syntax is extremely simple:
* <tt>DynamicAudioNormalizerCLI.exe -i *input_file* -o *output_file* [options]</tt>

Note that the *input* and *output* files always need to be specified, while the rest is optional. Existing output files will be *overwritten*!

Also note that the Dynamic Audio Normalizer program uses [libsndfile](http://www.mega-nerd.com/libsndfile/) for input/output, so it can deal with a wide range of file formats (WAV, W64, AIFF, AU, etc) and various sample types (8-Bit/16-Bit/24-Bit Integer, 32-Bit/64-Bit Float, ADPCM, etc).

**Example:**
* <tt>DynamicAudioNormalizerCLI.exe -i "c:\users\john doe\my music\original.wav" -o "c:\users\john doe\my music\normalized.wav"</tt>

For a list of available options, please run '<tt>DynamicAudioNormalizerCLI.exe --help</tt>' or see the following chapter…

-------------------------------------------------------------------------------
Configuration
-------------------------------------------------------------------------------

This chapter describes the configuration options that can be used to tweak the behaviour of the Dynamic Audio Normalizer.

While the default parameter of the Dynamic Audio Normalizer have been chosen to give satisfying results with a wide range of audio sources, it can be advantageous to adapt the parameters to the individual audio file as well as to your personal preferences. 

<br>
### Gaussian Filter Window Size ###

The most important parameter of the Dynamic Audio Normalizer is the "window size" of the Gaussian smoothing filter. It can be controlled with the **'<tt>--gauss-size</tt>'** option. The filter's window size is specified in *frames*, centered around the current frame. For the sake of simplicity, this must be an odd number. Consequently, the default value of **31** takes into account the current frame, as well as the *15* preceding frames and the *15* subsequent frames. Using a *larger* window results in a *stronger* smoothing effect and thus in *less* gain variation, i.e. slower gain adaptation. Conversely, Using a *smaller* window results in a *weaker* smoothing effect and thus in *more* gain variation, i.e. faster gain adaptation. The following graph illustrates the effect of different filter sizes – *11* (orange), *31* (green), and *61* (purple) frames – on the progression of the final filtered gain factor.

![FilterSize](img/FilterSize.png "Dynamic Audio Normalizer – Filter Size Effects")  
<small>**Figure 4:** The effect of different "window sizes" of the Gaussian smoothing filter.</small>

<br>
### Target Peak Magnitude ###

The target peak magnitude specifies the highest permissible magnitude level for the *normalized* audio file. It is controlled by the **'<tt>--peak</tt>'** option. Since the Dynamic Audio Normalizer represents audio samples as floating point values in the *-1.0* to *1.0* range – regardless of the input and output audio format – this value must be in the *0.0* to *1.0* range. Consequently, the value *1.0* is equal to [0 dBFS](http://en.wikipedia.org/wiki/DBFS), i.e. the maximum possible digital signal level (± 32767 in a 16-Bit file). The Dynamic Audio Normalizer will try to approach the target peak magnitude as closely as possible, but at the same time it also makes sure that the normalized signal will *never* exceed the peak magnitude. A frame's maximum local gain factor is imposed directly by the target peak magnitude. The default value is **0.95** and thus leaves a [headroom](http://en.wikipedia.org/wiki/Headroom_%28audio_signal_processing%29) of *5%*. It is **not** recommended to go *above* this value!

<br>
### Channel Coupling ###

By default, the Dynamic Audio Normalizer will amplify all channels by the same amount. This means the *same* gain factor will be applied to *all* channels and thus the maximum possible gain factor is determined by the "loudest" channel. In particular, the highest magnitude sample for the <tt>n</tt>-th frame is defined as <tt>S_max[n]=Max(s_max[n][1],s_max[n][2],…,s_max[n][C])</tt>, where <tt>s_max[n][k]</tt> denotes the highest magnitude sample in the <tt>k</tt>-th channel and <tt>C</tt> is the number of channels. The gain factor for *all* channels is then derived from <tt>S_max[n]</tt>. This is referred to as *channel coupling* and for most audio files it should give the desired result. Therefore, channel coupling is *enabled* by default. However, in some recordings, it may happen that the volume of the different channels is uneven, e.g. one channel may be "quieter" than the other one(s). In this case, the **<tt>--no-coupling</tt>** option can be used to *disable* the channel coupling. This way, the gain factor will be determined *independently* for each channel <tt>k</tt>, depending only on the channel's highest magnitude sample <tt>s_max[n][k]</tt>. This allows for harmonizing the volume of the different channels.

<br>
### DC Bias Correction ###

<tt>--correct-dc</tt>
 
<br>
### Maximum Gain Factor ###

<tt>--max-gain</tt>

<br>
### Frame Length ###

<tt>--frame-len</tt>

-------------------------------------------------------------------------------
API Documentation
-------------------------------------------------------------------------------

This chapter describes the **MDynamicAudioNormalizer** class, as defined in the <tt>DynamicAudioNormalizer.h</tt> header file. It allows software developer to call the Dynamic Audio Normalizer library from their own application code.

<br>
### MDynamicAudioNormalizer::MDynamicAudioNormalizer() ###
```
MDynamicAudioNormalizer(
	const uint32_t channels,
	const uint32_t sampleRate,
	const uint32_t frameLenMsec,
	const bool channelsCoupled,
	const bool enableDCCorrection,
	const double peakValue,
	const double maxAmplification,
	const uint32_t filterSize,
	const bool verbose = false,
	FILE *const logFile = NULL
);
```

Constructor. Creates a new MDynamicAudioNormalizer instance and sets up the normalization parameters.

**Parameters:**
* *channels*: The number of channels in the input/output audio stream (e.g. **2** for Stereo).
* *sampleRate*: The sampling rate of the input/output audio stream, in Hertz (e.g. **44100** for "CD Quality").
* *frameLenMsec*: The frame length, in milliseconds. A typical value is **500** milliseconds.
* *channelsCoupled*: Set to **true** in order to enable channel coupling, or to **false** otherwise (default: **true**).
* *enableDCCorrection*: Set to **true** in order to enable DC correction, or to **false** otherwise (default: **false**).
* *peakValue*: Specifies the peak magnitude for normalized audio, in the **0.0** to **1.0** range (default: **0.95**).
* *maxAmplification*: Specifies the maximum amplification factor. Must be greater than **1.0** (default: **10.0**).
* *filterSize*: The "window size" of the Gaussian filter, in frames. Must be an *odd* number. (default: **31**).
* *verbose*: Set to **true** in order to enable additional diagnostic logging, or to **false** otherwise (default: **false**).
* *logFile*: An open **FILE*** handle with *write* access that receives the logging info, or **NULL** to disable logging.

<br>
### MDynamicAudioNormalizer::~MDynamicAudioNormalizer() ###
```
virtual ~MDynamicAudioNormalizer(void);
```

Destructor. Destroys the MDynamicAudioNormalizer instance and releases all memory that it occupied.

<br>
### MDynamicAudioNormalizer::initialize() ###
```
bool initialize(void);
```

Initializes the MDynamicAudioNormalizer instance. Validates the parameters and allocates/initializes the required memory buffers.

This function *must* be called once for each new MDynamicAudioNormalizer instance. It *must* be called before <tt>processInplace()</tt> or <tt>setPass()</tt> are called.

**Return value:**
* Returns <tt>true</tt> if everything was successfull or <tt>false</tt> if something went wrong.

<br>
### MDynamicAudioNormalizer::setPass() ###
```
bool setPass(
	const int pass
);
```

Starts a new processing pass and *must* be called before <tt>processInplace()</tt> can be called. The previous pass is ended *implicitly* when a new one is started.

The application is supposed to start the *first* pass, then process all input samples (by calling <tt>processInplace()</tt> in a loop), then start the *second* pass and finally process all input samples again (by calling <tt>processInplace()</tt> in a loop).

During the *first* pass, output samples are returned but these samples will **not** be normalized. The application will usually *discard* the output of the *first* pass and only retain the output of the *second* pass.

**Parameters:**
* *pass*: This value can be either <tt>MDynamicAudioNormalizer::PASS_1ST</tt> or <tt>MDynamicAudioNormalizer::PASS_2ND</tt> in order to start the *first* or *second* pass, respectively.

**Return value:**
* Returns <tt>true</tt> if everything was successfull or <tt>false</tt> if something went wrong.

<br>
### MDynamicAudioNormalizer::processInplace() ###
```
bool processInplace(
	double **samplesInOut,
	int64_t inputSize,
	int64_t &outputSize
);
```

This is the main processing function. It usually is called in a loop by the application until all audio samples (for the *current* pass) have been processed. It is very important that all input samples are **identical** for the *first* and the *second* pass and that the total sample count does **not** change!

The function works "in place": It *reads* the original input samples from the specified buffer and then *writes* the normalized output samples, if any, back into the *same* buffer. The content of <tt>samplesInOut</tt> will **not** be preserved!

It's possible that a specific call to this function returns *fewer* output samples than the number of input samples that have been read! The pending samples are buffered internally and will be returned in a subsequent function call. This also means that the *i*-th output sample doesn't necessarily match *i*-th input sample. However, the samples are always returned in a strict FIFO (first-in, first-out) order. At the end of a pass, when all input smaples have been read, the application may feed this function with "dummy" (null) samples in order to *flush* all pending output samples.

**Parameters:**
* *samplesInOut*: The buffer that contains the original input samples and that will receive the normalized output samples. The *i*-th input sample for the *c*-th channel is assumed to be stored at <tt>samplesInOut[c][i]</tt>, as a double-precision floating point number in the **-1.00** to **1.00** range. All indices are zero-based. The output samples, if any, will be stored at the corresponding locations, thus *overwriting* the input data. Consequently, the *i*-th output sample for the *c*-th channel will be stored at <tt>samplesInOut[c][i]</tt> and is guaranteed to be inside the **-1.00** to **1.00** range.
* *inputSize*: The number of *input* samples that are available in the <tt>samplesInOut</tt> buffer. This also specifies the *maximum* number of output samples to be stored in the buffer.
* *outputSize*: Receives the number of *output* samples that have been stored in the <tt>samplesInOut</tt> buffer. Please note that this value can be *smaller* than <tt>inputSize</tt> size. It can even be *zero*!

**Return value:**
* Returns <tt>true</tt> if everything was successfull or <tt>false</tt> if something went wrong.

<br>
### MDynamicAudioNormalizer::getVersionInfo() ###
```
static void getVersionInfo(
	uint32_t &major,
	uint32_t &minor,
	uint32_t &patch
);
```

This *static* function can be called to determine the Dynamic Audio Normalizer library version.

**Parameters:**
* *major*: Receives the major version number. Value will currently be **1**.
* *minor*: Receives the minor version number. Value will be in the **0** to **99** range.
* *patch*: Receives the patch level. Value will be in the **0** to **9** range.

<br>
### MDynamicAudioNormalizer::getBuildInfo() ###
```
static void getBuildInfo(
	const char **date,
	const char **time,
	const char **compiler,
	const char **arch,
	bool &debug
);
```

This *static* function can be called to determine more detailed information about the specific Dynamic Audio Normalizer build.

**Parameters:**
* *date*: Receives a pointer to a *read-only* string buffer containing the build date. Uses standard <tt>__DATE__</tt> format.
* *time*: Receives a pointer to a *read-only* string buffer containing the build time. Uses standard <tt>__TIME__</tt> format.
* *compiler*: Receives a pointer to a *read-only* string buffer containing the compiler identifier (e.g. "MSVC 2013.2").
* *arch*: Receives a pointer to a *read-only* string buffer containing the architecture identifier (e.g. "x86" for IA32/x86 or "x64" for AMD64/EM64T).
* *debug*: Will be set to <tt>true</tt> if this is a *debug* build or to <tt>false</tt> otherwise. Don't use the *debug* version production!

-------------------------------------------------------------------------------
License Terms
-------------------------------------------------------------------------------

```
Dynamic Audio Normalizer - Audio Processing Utility
Copyright (c) 2014 LoRd_MuldeR <mulder2@gmx.de>. Some rights reserved.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
```

http://www.gnu.org/licenses/gpl-2.0.html

-------------------------------------------------------------------------------

e.o.f.
