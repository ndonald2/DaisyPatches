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
            
            sin_oscs_[i].Init(sr);
            sin_oscs_[i].SetAmp(0.9);
            sin_oscs_[i].SetWaveform(Oscillator::WAVE_SIN);
        }

        button_.Init(hw.B7);
        switch_.Init(hw.B8);
        smooth_coef_ = 1.0 / (sr * 0.001);
    }

    void ProcessSwitches()
    {
        button_.Debounce();
        switch_.Debounce();

        if (button_.RisingEdge())
        {
            nextWaveMode();
        } else if (button_.TimeHeldMs() > 1000) {
            waveMode_ = 0;
            updateWaveforms();
        }
    }

    void Update(DaisyPatchSM &hw)
    {
        // -- Pitch --
        bool harmonic_mode = switch_.Pressed();

        // Knobs
        float base_in = roundf(fmap(hw.GetAdcValue(CV_1), 28.0, 84.0)); // E1 - C6
        float offset_in = harmonic_mode ? roundf(fmap(hw.GetAdcValue(CV_2), -2, 15.0)) : roundf(fmap(hw.GetAdcValue(CV_2), -24.0, 24.0));
        float detune_in = fmap(hw.GetAdcValue(CV_3), 0.0, 0.25);

        // CV Ins
        base_in += hw.GetAdcValue(CV_5) * 60.0; // 5 octaves plus/minus
        offset_in += round(hw.GetAdcValue(CV_6) * 24.0);

        // Post-map smoothing
        fonepole(base_, base_in, smooth_coef_);
        fonepole(offset_, offset_in, smooth_coef_);

        float base_freq = mtof(base_ - detune_in);
        oscs_[0].SetFreq(base_freq);
        sin_oscs_[0].SetFreq(base_freq);

        if (harmonic_mode)
        {
            float scale = offset_in >= 0.0 ? offset_in + 1.0 : 1.0 / -(offset_in * 2.0);
            float scaled_freq = mtof(base_ + detune_in) * scale;
            oscs_[1].SetFreq(scaled_freq);
            sin_oscs_[1].SetFreq(scaled_freq);
        }
        else
        {
            float interval_freq = mtof(base_ + offset_ + detune_in);
            oscs_[1].SetFreq(interval_freq);
            sin_oscs_[1].SetFreq(interval_freq);
        }

        // -- Color --
        float color_in = fclamp(hw.GetAdcValue(CV_4) + hw.GetAdcValue(CV_8), 0.0, 1.0);
        oscs_[0].SetPw(color_in);
        oscs_[1].SetPw(color_in);
    }

    void FillBuffers(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
    {
        float ss1 = osc1SinScale();
        float ss2 = osc2SinScale();

        for (size_t i = 0; i < size; i++)
        {
            OUT_L[i] = (1.0 - ss1) * oscs_[0].Process() + ss1 * sin_oscs_[0].Process();
            OUT_R[i] = (1.0 - ss2) * oscs_[1].Process() + ss2 * sin_oscs_[1].Process();
        }
    }

private:
    enum WaveModes
    {
        MODE_SIN_SIN,
        MODE_TRI_TRI,
        MODE_SAW_SAW,
        MODE_RECT_RECT,
        MODE_SAW_SIN,
        MODE_SAW_TRI,
        MODE_SAW_RECT,
        MODE_RECT_SIN,
        MODE_RECT_TRI,
        MODE_RECT_SAW,
        MODE_LAST
    };

    BlOsc oscs_[2];
    Oscillator sin_oscs_[2];

    Switch button_;
    Switch switch_;

    uint8_t waveMode_ = 0;

    float base_;
    float offset_;
    float smooth_coef_;

    float osc1SinScale()
    {
        return waveMode_ == MODE_SIN_SIN ? 1.0 : 0.0;
    }

    float osc2SinScale()
    {
        switch (waveMode_) {
            case MODE_SIN_SIN:
            case MODE_SAW_SIN:
            case MODE_RECT_SIN:
                return 1.0;
            default:
                return 0.0;
        }
    }

    void nextWaveMode()
    {
        waveMode_ = (waveMode_ + 1) % MODE_LAST;
        updateWaveforms();
    }

    void updateWaveforms() {
        switch (waveMode_)
        {
        case MODE_SIN_SIN:
            break;

        case MODE_TRI_TRI:
            oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_TRIANGLE);
            oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_TRIANGLE);
            break;

        case MODE_SAW_SAW:
        case MODE_SAW_SIN:
            oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_SAW);
            oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_SAW);
            break;

        case MODE_SAW_TRI:
            oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_SAW);
            oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_TRIANGLE);
            break;

        case MODE_SAW_RECT:
            oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_SAW);
            oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_SQUARE);
            break;

        case MODE_RECT_RECT:
        case MODE_RECT_SIN:
            oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_SQUARE);
            oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_SQUARE);
            break;

        case MODE_RECT_TRI:
            oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_SQUARE);
            oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_TRIANGLE);
            break;

        case MODE_RECT_SAW:
            oscs_[0].SetWaveform(BlOsc::Waveforms::WAVE_SQUARE);
            oscs_[1].SetWaveform(BlOsc::Waveforms::WAVE_SAW);
            break;
        }
    }
};

DaisyPatchSM hw;
IntervalOscPatch intervalOsc;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
    hw.ProcessAllControls();
    intervalOsc.Update(hw);
    intervalOsc.FillBuffers(in, out, size);
}

int main(void)
{
    hw.Init();
    hw.SetAudioBlockSize(DSP_BLK_SIZE); // number of samples handled per callback
    hw.SetAudioSampleRate(DSP_SR);
    hw.StartAudio(AudioCallback);

    intervalOsc.Init(hw);

    // hw.StartLog(true);

    while (1)
    {
        // Process physical switch/button at slower interval
        // bc doing so at audio rate outruns debounce prevention algo
        intervalOsc.ProcessSwitches();
        System::Delay(1);
    }
}
