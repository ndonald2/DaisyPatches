#include <daisy_patch_sm.h>
#include <daisysp.h>

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

enum channel {
	LEFT,
	RIGHT
};

DaisyPatchSM hw;
Oscillator lfo[2];
float rate;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	hw.ProcessAllControls();

	const float rate_in = fmap(hw.GetAdcValue(CV_1), 0.1f, 20.f, Mapping::EXP);
	const float depth = fastroot(daisysp::fmax(0.0001f, hw.GetAdcValue(CV_2)), 2);

	fonepole(rate, rate_in, 0.00008333f); // hard-coded tau = 0.25s for smoothing

	lfo[LEFT].SetFreq(rate);
	lfo[RIGHT].SetFreq(rate);
	
	for (size_t i = 0; i < size; i++)
	{
		// TODO: stereo width via out of phase LFO
		// (will require different LFO class than daisysp)
		float lfo_l = lfo[LEFT].Process() * 0.5f + 0.5f;
		float lfo_r = lfo[RIGHT].Process() * 0.5f + 0.5f;

		OUT_L[i] = IN_L[i] * (1.0f - lfo_l * depth);
		OUT_R[i] = IN_R[i] * (1.0f - lfo_r * depth);
	}
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

	lfo[LEFT].Init(hw.AudioSampleRate());
	lfo[LEFT].SetWaveform(Oscillator::WAVE_SIN);
	lfo[LEFT].SetAmp(1.0f);

	lfo[RIGHT].Init(hw.AudioSampleRate());
	lfo[RIGHT].SetWaveform(Oscillator::WAVE_SIN);
	lfo[RIGHT].SetAmp(1.0f);

	hw.StartAudio(AudioCallback);

	while(1) {}
}
