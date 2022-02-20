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
			float sr = hw.AudioSampleRate();

			for (size_t i = 0; i < 2; i++)
			{
				oscs_[i].Init(sr);
				oscs_[i].SetAmp(0.9);
				oscs_[i].SetWaveform(BlOsc::Waveforms::WAVE_SAW);
			}

			osc_sub_.Init(sr);
			osc_sub_.SetAmp(0.9);
			osc_sub_.SetWaveform(BlOsc::Waveforms::WAVE_TRIANGLE);

			button_.Init(hw.B7);
			switch_.Init(hw.B8);
			smooth_coef_ = 1.0 / (sr * 0.002);
		}

		void Update(DaisyPatchSM &hw)
		{
			// -- Wave Mode --
			button_.Debounce();
			switch_.Debounce();

			if (button_.RisingEdge())
			{
				nextWaveMode();	
			}

			// -- Pitch --

			// Knobs
			float base_nn_in = roundf(fmap(hw.GetAdcValue(CV_1), 33.0, 81.0)); // A1 - A5
			float offset_nn_in = roundf(fmap(hw.GetAdcValue(CV_2), -24.0, 24.0));
			float detune_in = fmap(hw.GetAdcValue(CV_3), 0.0, 0.2);

			// CV Ins
			base_nn_in += hw.GetAdcValue(CV_5) * 60.0; // 5 octaves plus/minus
			offset_nn_in += round(hw.GetAdcValue(CV_6) * 24.0);

			// Post-map smoothing
			fonepole(base_nn_, base_nn_in, smooth_coef_);
			fonepole(offset_nn_, offset_nn_in, smooth_coef_);

			oscs_[0].SetFreq(mtof(base_nn_ - detune_in));
			oscs_[1].SetFreq(mtof(base_nn_ + offset_nn_ + detune_in));
			osc_sub_.SetFreq(mtof(base_nn_ - 12));

			// -- Color --

		}

		void FillBuffers(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
		{
			// Sum + Subosc
			if (switch_.Pressed())
			{
				for (size_t i = 0; i < size; i++)
				{
					OUT_L[i] = (oscs_[0].Process() + oscs_[1].Process()) * 0.5; 
					OUT_R[i] = osc_sub_.Process();
				}
			}
			// Separate outs
			else
			{
				for (size_t i = 0; i < size; i++)
				{
					OUT_L[i] = oscs_[0].Process(); 
					OUT_R[i] = oscs_[1].Process();
				}
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
		BlOsc osc_sub_;

		Switch button_;
		Switch switch_;

		uint8_t waveMode_ = 0;

		bool has_init_pitches_ = false;
		float base_nn_;
		float offset_nn_;
		float smooth_coef_;

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

	// hw.StartLog(true);

	while (1) {}
}
