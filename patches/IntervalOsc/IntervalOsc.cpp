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

		void Init(DaisyPatchSM &hw)
		{
			button_.Init(hw.B7);
			for (size_t i = 0; i < 2; i++) {
				oscs_[i].Init(hw.AudioSampleRate());
				oscs_[i].SetAmp(0.5);
				oscs_[i].SetWaveform(BlOsc::Waveforms::WAVE_SAW);
			}
		}

		void Update(DaisyPatchSM &hw) {
			// -- Wave Mode --
			button_.Debounce();
			if (button_.RisingEdge()) {
				nextWaveMode();	
			}

			// -- Pitch --
			float base_nn = roundf(fmap(hw.GetAdcValue(CV_1), 21, 81));
			float offset_nn = roundf(fmap(hw.GetAdcValue(CV_2), -12, 12));
			oscs_[0].SetFreq(mtof(base_nn));
			oscs_[1].SetFreq(mtof(base_nn + offset_nn));

			// -- Color --
		}

		void FillBuffers(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
			for (size_t i = 0; i < size; i++) {
				OUT_L[i] = oscs_[0].Process(); 
				OUT_R[i] = oscs_[1].Process();
			}
		}

	private:
		enum WaveModes
		{
			MODE_SAW_SAW,
			MODE_RECT_RECT,
			MODE_TRI_TRI,
			MODE_SAW_RECT,
			MODE_SAW_TRI,
			MODE_RECT_TRI,
			MODE_LAST
		};
		BlOsc oscs_[2];
		Switch button_;
		uint8_t waveMode_ = 0;

		void nextWaveMode() {
			waveMode_ = (waveMode_ + 1) % MODE_LAST;
			switch (waveMode_) {
				case MODE_SAW_SAW:
					oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_SAW);
					oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_SAW);
					break;
				
				case MODE_RECT_RECT:
					oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_SQUARE);
					oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_SQUARE);
					break;

				case MODE_TRI_TRI:
					oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_TRIANGLE);
					oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_TRIANGLE);
					break;

				case MODE_SAW_RECT:
					oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_SAW);
					oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_SQUARE);
					break;

				case MODE_SAW_TRI:
					oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_SAW);
					oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_TRIANGLE);
					break;

				case MODE_RECT_TRI:
					oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_SQUARE);
					oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_TRIANGLE);
					break;
			}
		}
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

	patch.Init(hw);

	while (1) {}
}
