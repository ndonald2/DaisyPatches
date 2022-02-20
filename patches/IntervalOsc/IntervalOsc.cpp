#include <functional>
#include "daisy_patch_sm.h"
#include "daisysp.h"

#define DSP_BLK_SIZE 4
#define DSP_SR SaiHandle::Config::SampleRate::SAI_48KHZ

using namespace daisy;
using namespace patch_sm;
using namespace daisysp;

class IntervalOscPatch
{
	public:
		IntervalOscPatch() {}

		void Init(float sample_rate)
		{
			for (size_t i = 0; i < 2; i++) {
				oscs_[i].Init(sample_rate);
				oscs_[i].SetAmp(0.5);
				oscs_[i].SetWaveform(BlOsc::Waveforms::WAVE_SAW);
				oscs_[i].SetFreq(mtof(50));
			}
		}

		void Update(DaisyPatchSM &hw) {
			float base_nn = roundf(fmap(hw.GetAdcValue(CV_1), 21, 81));
			oscs_[0].SetFreq(mtof(base_nn));
			oscs_[1].SetFreq(mtof(base_nn));
		}

		void FillBuffers(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
			for (size_t i = 0; i < size; i++) {
				OUT_L[i] = oscs_[0].Process(); 
				OUT_R[i] = oscs_[1].Process();
			}
		}

	private:
		BlOsc oscs_[2];
};

DaisyPatchSM hw;
IntervalOscPatch patch;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	hw.ProcessAllControls();
	patch.Update(hw);
	patch.FillBuffers(in, out, size);
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(DSP_BLK_SIZE); // number of samples handled per callback
	hw.SetAudioSampleRate(DSP_SR);
	hw.StartAudio(AudioCallback);

	patch.Init(hw.AudioSampleRate());

	while (1) {}
}
