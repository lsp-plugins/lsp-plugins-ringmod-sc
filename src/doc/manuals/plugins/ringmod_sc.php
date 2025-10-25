<?php
	plugin_header();
	$m      =   ($PAGE == 'ringmod_sc_mono') ? 'm' : 's';
?>

<p>This plugins performs ring-modulated sidechaing of the audio signal.</p>
<p>The simplified process can be described by following steps:</p>
<ul>
<li>Rectifying the sidechain signal. All negative audio samples on sidechain input change their sign and become positive.</li>
<li>Limiting the sidechain signal. All audio samples of the sidechain signal that come from previous step are limited by the 0 dBFS threshold.</li>
<li>Modulating the original signal. The original signal is multiplied by rectified and limited sidechain signal.</li>
<li>Subtracting from the original signal. The modulated result is subtracted from the original signal.</li>
</ul>
<p>Such algorithm allows to mix two signals with 0 dBFS peak values without going out of 0 dBFS range.</p>
<p>As a side effect, because sidechain signal can change rapidly, the original signal can become distorted. To minimize distortion
effect, additional smoothing, prediction and reactivity controls were added.</p>  

<p><b>Controls:</b></p>
<ul>
	<li>
		<b>Bypass</b> - bypass switch, when turned on (led indicator is shining), the output signal is similar to input signal. That does not mean
		that the plugin is not working.
	</li>
	<li><b>Pause</b> - pauses any updates of the time graph.</li>
	<li><b>Clear</b> - clears all graphs.</li>
	<li><b>Active</b> - enables side-chaining effect. May be useful for A/B testing the signal with sidechain effect and without it.</li>
	<li><b>Invert</b> - enables inverse function: in this mode the plugin works like a ring modulator instead of rin-modulated sidechain.</li>
	<li><b>Type</b> - The sidechain source type:</li>
	<ul>
		<li><b>Internal</b> - the input signal is taken as a sidechain after pre-mixing stage.</li>
		<li><b>External</b> - the sidechain input signal is taken as a sidechain after pre-mixing stage.</li>
		<li><b>Link</b> - the shared memory link is used as a sidechain input signal after pre-mixing-stage.</li>
	</ul>
	<?php if ($m == 's') { ?>
	<li><b>Source</b> - The sidechain source type for both left and right channels:</li>
	<ul>
		<li><b>Left/Right</b> the left and right channels are processed using the left and right channels of sidechain respectively.</li>
		<li><b>Right/Left</b> the left and right channels are processed using the right and left channels of sidechain respectively.</li>
		<li><b>Left</b> both the left and right channels are processed using only the left channel of sidechain.</li>
		<li><b>Right</b> both the left and right channels are processed using only the right channel of sidechain.</li>
		<li><b>Mid/Side</b> the left and right channels are processed using the middle and side signal repesentation of sidechain respectively.</li>
		<li><b>Side/Mid</b> the left and right channels are processed using the side and middle signal repesentation of sidechain respectively.</li>
		<li><b>Mid</b> both the left and right channels are processed using only the middle signal repesentation of sidechain respectively.</li>
		<li><b>Side</b> both the left and right channels are processed using only the side signal repesentation of sidechain respectively.</li>
		<li><b>Min</b> both the left and right channels are processed using the minimum signal between left and right channels of sidechain.</li>
		<li><b>Max</b> both the left and right channels are processed using the maximum signal between left and right channels of sidechain.</li>
	</ul>
	<?php } ?>
	<li><b>Pre-mix</b> - shows pre-mix control overlay.</li>
	<li><b>Mix</b> - shows the Dry/Wet control overlay.</li>
	<li><b>Link</b> - the name of the shared memory link to pass sidechain signal.</li>
	<li><b>SC</b> - enables drawing of sidechain input graph and corresponding level meter.</li>
	<li><b>Gain</b> - enables drawing of gain amplification line and corresponding amplification meter.</li>
	<li><b>In</b> - enables drawing of input signal graph and corresponding level meter.</li>
	<li><b>Out</b> - enables drawing of output signal graph and corresponding level meter.</li>
</ul>

<p><b>Processor</b> section:</p>
<ul>
	<li><b>Lookahead</b> - allows to add some small delay to the signal and force sidechain to reduce the input signal earlier than actual peak happens.</li>
	<li><b>Ducking</b> - allows to add some small post-delay to the sidechain signal to force slower shutdown of the sidechain signal.</li>
	<li><b>Hold</b> - the time period the sidechain envelope holds it's maximum value before starting the release.</li>
	<li><b>Release</b> - the release time of the sidechain.</li>
	<li><b>Amount</b> - the additional pre-amplification of the sidechain signal before the limiting stage.</li>
	<?php if ($m == 's') { ?>
	<li><b>Stereo link</b> - the knob that allows to set how the gain reduction of the left channel affects the gain reduction of the right channel and vice verse.</li>
	<?php } ?>
</ul>

<p><b>Signal</b> section:</p>
<ul>
	<li><b>Input</b> button - allows to pass the processed input signal tho the output of plugin.</li>
	<li><b>Input</b> knob - the loudness of the processed input signal.</li>
	<li><b>Sidechain</b> button - allows to pass the unprocessed sidechain signal tho the output of plugin.</li>
	<li><b>Sidechain</b> knob - the loudness of the unprocessed sidechain signal.</li>
	<li><b>Output</b> - the overall loudness of output signal.</li>
</ul>

<p><b>Pre-mix</b> control overlay:</p>
<ul>
	<li><b>In -> SC</b> - the amount of signal from input channel added to the Sidechain.</li>
	<li><b>In -> Link</b> - the amount of signal from input channel added to the shared memory link.</li>
	<li><b>SC -> In</b> - the amount of signal from sidechain input channel added to the input channel.</li>
	<li><b>SC -> Link</b> - the amount of signal from sidechain input channel added to the shared memory link.</li>
	<li><b>Link -> In</b> - the amount of signal from shared memory link added to the input channel.</li>
	<li><b>Link -> SC</b> - the amount of signal from shared memory link added to the sidechain channel.</li>
</ul>

<p><b>Dry/Wet mix</b> control overlay:</p>
<ul>
	<li><b>Mix Dry</b> - the amount of dry (unprocessed) signal.</li>
	<li><b>Mix Wet</b> - the amount of wet (processed) signal.</li>
	<li><b>Mix Dry/Wet</b> - the knob that controls this balance between mixed dry and wet signal (see <b>Mix Dry</b> and <b>Mix Wet</b>) and the dry (unprocessed) signal.</li>
</ul>
