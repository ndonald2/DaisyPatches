#include <functional>
#include "daisy_patch_sm.h"
#include "daisysp.h"

#define DSP_BLK_SIZE 4 
#define DSP_SR SaiHandle::Config::SampleRate::SAI_96KHZ

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

			button_.Init(hw.B7);
			switch_.Init(hw.B8);
			smooth_coef_ = 1.0 / (sr * 0.001);
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

			bool harmonic_mode = switch_.Pressed();

			// Knobs
			float base_in = roundf(fmap(hw.GetAdcValue(CV_1), 28.0, 84.0)); // E1 - C6 
			float offset_in = harmonic_mode ? 
				roundf(fmap(hw.GetAdcValue(CV_2), -2, 15.0)) :
				roundf(fmap(hw.GetAdcValue(CV_2), -24.0, 24.0));
			float detune_in = fmap(hw.GetAdcValue(CV_3), 0.0, 0.2);

			// CV Ins
			base_in += hw.GetAdcValue(CV_5) * 60.0; // 5 octaves plus/minus
			offset_in += round(hw.GetAdcValue(CV_6) * 24.0);

			// Post-map smoothing
			fonepole(base_, base_in, smooth_coef_);
			fonepole(offset_, offset_in, smooth_coef_);

			oscs_[0].SetFreq(mtof(base_ - detune_in));
			if (harmonic_mode)
			{
				float scale = offset_in >= 0.0 ? offset_in + 1.0 : 1.0 / -(offset_in * 2.0);
				oscs_[1].SetFreq(mtof(base_ + detune_in) * scale);
			}
			else
			{
				oscs_[1].SetFreq(mtof(base_ + offset_ + detune_in));
			}

			// -- Color --
			float color_in = fclamp(hw.GetAdcValue(CV_4) + hw.GetAdcValue(CV_8), 0.0, 1.0);
			oscs_[0].SetPw(color_in);
			oscs_[1].SetPw(color_in);
		}

		void FillBuffers(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
		{
			for (size_t i = 0; i < size; i++)
			{
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
		Switch switch_;

		uint8_t waveMode_ = 0;

		bool has_init_pitches_ = false;
		float base_;
		float offset_;
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
