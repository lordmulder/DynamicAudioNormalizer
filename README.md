Dynamic Audio Normalizer
===============================================================================

<small>Created by LoRd_MuldeR <<mulder2@gmx>></tt> – Please check http://muldersoft.com/ for news and updates!</small>

**Dynamic Audio Normalizer** is a library and a command-line tool for *audio normalization*. It applies a certain amount of gain to the input audio in order to bring its peak magnitude to a target level (e.g. 0 dBFS). However, in contrast to more "simple" normalization algorithms, the Dynamic Audio Normalizer *dynamically* adjusts the gain factor to the input audio. This allows for applying extra gain to the "quiet" parts of the audio while avoiding distortions or clipping the "loud" parts. In other words, the volume of the "quiet" and the "loud" parts will be harmonized.

-------------------------------------------------------------------------------
How it works
-------------------------------------------------------------------------------

The "standard" audio normalization algorithm applies the same *constant* amount of gain to *all* samples in the file. Consequently, the gain factor must be chosen in a way that won't cause clipping/distortion – even for the input sample that has the highest magnitude. So if <tt>S_max</tt> denotes the highest magnitude sample in the input audio and <tt>Peak</tt> is the desired peak magnitude, then the gain factor will be chosen as <tt>G=Peak/abs(S_max)</tt>. This works fine, as long as the volume of the input audio remains constant, more or less, all the time. If, however, the volume of the input audio varies significantly over time – as is the case with many "real world" recordings – the standard normalization algorithm will *not* give satisfying result. That's because the "loud" parts can *not* be amplified any further (without distortions) and thus the "quiet" parts will remain quiet too.

Dynamic Audio Normalizer solves this problem by processing the input audio in small chunks, referred to as *frames*. A frame typically has a length 500 milliseconds, but the frame size can be adjusted as needed. It then finds the highest magnitude sample *within* each frame. Finally it computes the maximum possible gain factor (without distortions) for each individual frame. So if <tt>S_max[n]</tt> denotes the highest magnitude sample within the <tt>n</tt>-th frame, then the maximum possible gain factor for the <tt>n</tt>-th frame will be <tt>G[n]=Peak/abs(S_max[n])</tt>. Unfortunately, simply amplifying each frame with its own "local" maximum gain factor <tt>G[n]</tt> would *not* give satisfying results either. That's because the maximum gain factors can vary *strongly* and *unsteadily* between neighbouring frames! Therefore, applying the maximum possible gain to each frame *without* taking neighbouring frames into account would essentially result in a strong *dynamic range compression* – which not only has a tendency to destroy the "vividness" of the audio but can also result in the nasty "pumping" effect, i.e fast changes of the gain factor that become clearly noticeable to the listener.

The Dynamic Audio Normalizer tries to avoid these issues by applying an advanced *2-Pass* normalization algorithm. Essentially, while the *first* pass computes (and stores) the maximum "local" gain factor <tt>G[n]</tt> for each individual frame, the actual normalization takes place in the *second* pass. Before the second pass starts, a [*Gaussian* smoothing kernel](http://en.wikipedia.org/wiki/Gaussian_blur) will be applied to the gain factors. Put simply, this filter "mixes" the gain factor of the <tt>n</tt>-th frames with those of all its preceding frames (<tt>n-1</tt>, <tt>n-2</tt>, &hellip;) as well as with all its subsequent frames (<tt>n+1</tt>, <tt>n+2</tt>, &hellip;) – where "near by" frames, in both directions, have a stronger influence, while "far away" frames, in both directions, have a declining influence. This way, abrupt changes of the gain factor are avoided and, instead, we get *smooth transitions* of the gain factor over time. Furthermore, since the filter also takes into account *future* frames, Dynamic Audio Normalizer avoids applying strong gain to "quiet" frames located shortly before "loud" frames. This can also be understood as a *lookahead* function which adjusts the gain factor *early* and thus nicely prevents clipping/distortion or abrupt gain reductions.

Once more detail to consider is that applying the Gaussian smoothing kernel alone can *not* solve all problems. That's because the smoothing kernel will *not* only smoothen/delay *increasing* gain factors but also *declining* ones! If, for example, a very "loud" frame follows immediately after a sequence of "quiet" frames, the smoothing causes the gain factor to decrease early but slowly. As a result, the *filtered* gain factor of the "loud" frame could actually turn out to be *higher* than its (local) maximum gain factor – which results in distortion/clipping, if not taken care of! For this reason, the Dynamic Audio Normalizer *additionally* applies a "minimum" filter, i.e. a filter that replaces each gain factor with the *smallest* value within a certain neighbourhood. This is done *before* the Gaussian smoothing kernel in order to ensure that all gain transitions will remain smooth.

The following example shows the results form a "real world" audio recording that has been processed by the Dynamic Audio Normalizer. The chart shows the maximum local gain factors for each individual frame (blue) as well as the minimum filtered gain factors (green) and the final smoothend gain factors (orange). Note how smooth the progression of the final gain factors is, while approaching the maximum local gain factors as closely as possible. Also note how the smoothend gain factors *never* exceed the maximum local gain factor in order to avoid distortions.

![Chart](doc/Chart.png "Dynamic Audio Normalizer – Example")  
<small>**Figure 1:** Progression of the gain factors for each audio frame.</small>  
<br>

So far it has been discussed how the optimal gain factor for each frame is determined. However, since each frame contains a large number of samples – at a typical sampling rate of 44,100 Hz and a standard frame size of 500 milliseconds we have 22,050 samples per frame – it is also required to infer the gain factor for each individual sample in the frame. The most simple approach, of course, is applying the *same* gain factor to *all* samples in the certain frame. But this would lead to abrupt changes of the gain factor at each frame boundary, while the gain factor remains constant within the frames. A better approach, as implemented in the Dynamic Audio Normalizer, is interpolating the per-sample gain factors. In particular, the Dynamic Audio Normalizer applies a *linear interpolation* in order to compute the gain factors for the samples inside the <tt>n</tt>-th frame from the gain factors <tt>G'[n-1]</tt>, <tt>G'[n]</tt> and <tt>G'[n+1]</tt>, where <tt>G'[k]</tt> denotes the final *filtered* gain factor for the <tt>k</tt>-th frame. The following graph shows how the per-sample gain factors (orange) are interpolated from the gain factors of the preceding (green), current (blue) and subsequent (purple) frame.

![Interpolation](doc/Interpolation.png "Dynamic Audio Normalizer – Interpolation")  
<small>**Figure 2:** Linear interpolation of the per-sample gain factors.</small>  
<br>

Finally, the following waveform view illustrates how the volume of a "real world" audio recording has been harmonized by the Dynamic Audio Normalizer. The upper view shows the unprocessed original recording while the lower view shows the output as created by the Dynamic Audio Normalizer. As can be seen, the significant volume variation between the "loud" and the "quiet" parts that existed in the original recording has been rectified to a great extent, while retaining the dynamics of the input and avoiding clipping or distortion.

![Waveform](doc/Waveform.png "Dynamic Audio Normalizer – Example")  
<small>**Figure 3:** Waveform before and after processing.</small>

-------------------------------------------------------------------------------
Configuration
-------------------------------------------------------------------------------

![FilterSize](doc/FilterSize.png "Dynamic Audio Normalizer – Filter Size Effects")  
<small>**Figure 4:** The effect of different "window sizes" of the Gaussian smoothing filter.</small>

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
