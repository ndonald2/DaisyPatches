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
	const float width = fastpower(daisysp::fmax(0.0001f, hw.GetAdcValue(CV_3)), 2);
	const float out_scale = pow10f(-12.0f * depth * 0.05f);

	fonepole(rate, rate_in, 0.00008333f); // hard-coded tau = 0.25s for smoothing

	lfo[LEFT].SetFreq(rate);
	lfo[RIGHT].SetFreq(rate);
	lfo[RIGHT].SetPhase(lfo[LEFT].GetPhase() + width * PI_F);
	
for (size_t i = 0; i < size; i++)
{
	// unipolar 0-1
	float lfo_l = lfo[LEFT].Process() * 0.5f + 0.5f;
	float lfo_r = lfo[RIGHT].Process() * 0.5f + 0.5f;

	// decibel scale, -60db to 24dB at max depth
	lfo_l = pow10f((lfo_l * 42.0f - 18.0f) * depth * 0.05f);
	lfo_r = pow10f((lfo_r * 42.0f - 18.0f) * depth * 0.05f);


	OUT_L[i] = SoftClip(IN_L[i] * lfo_l) * out_scale;
	OUT_R[i] = SoftClip(IN_R[i] * lfo_r) * out_scale;
}
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_96KHZ);

	lfo[LEFT].Init(hw.AudioSampleRate());
	lfo[LEFT].SetWaveform(Oscillator::WAVE_SIN);
	lfo[LEFT].SetAmp(1.0f);

	lfo[RIGHT].Init(hw.AudioSampleRate());
	lfo[RIGHT].SetWaveform(Oscillator::WAVE_SIN);
	lfo[RIGHT].SetAmp(1.0f);

	hw.StartAudio(AudioCallback);

	while(1) {}
}
