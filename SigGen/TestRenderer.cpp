//
//  TestRenderer.cpp
//  SigGen
//
//  Created by Mike Erickson on 10/10/22.
//

#include "TestRenderer.hpp"
#include "base_waveforms.hpp"
#include "composite_waveforms.hpp"

std::shared_ptr<neato::ISampleSource> CreateFMBell(double center_freq, const neato::audio_stream_description_t& stream_desc_in)
{
    std::shared_ptr<neato::ISampleSource> carrier_temp = std::make_shared<neato::MutableSine>(center_freq, stream_desc_in.sample_rate);
    //frequency of the carrier gets modulated by a saw with a constant gain
    std::shared_ptr<neato::ISampleSource> saw_temp = std::make_shared<neato::ConstSaw>(1.4f * center_freq, stream_desc_in.sample_rate, false);
    std::shared_ptr<neato::ISampleSource> mod_gain_temp = neato::CreateConstantGain(160.0f);
    
    //make a modualted signal with the saw and the gain
    std::shared_ptr<neato::ISampleSource> saw_with_gain = std::make_shared<neato::ModulatedSignal>(saw_temp, std::shared_ptr<neato::ISampleSource>(), mod_gain_temp);
    //make a center frequency modulator function
    std::shared_ptr<neato::ICustomModulatorFunction> center_freq_mod = std::make_shared<neato::CenterFrequencyModulator>(center_freq);
    //then make the modulation sample source with the saw, the custom func and the gain (no frequency modulation on the saw. It PERFORMS frequency modulation on the mutable sine)
    std::shared_ptr<neato::ISampleSource> center_freq_saw_modulator = std::make_shared<neato::CustomModulator>(saw_with_gain, center_freq_mod);
    
    //make an envelope for the bell
    std::shared_ptr<neato::ISampleSource> bell_envelope = neato::CreateEnvelope(neato::EnvelopeID::Bell1, stream_desc_in.sample_rate, 1.0);
    
    //make an overall modulated signal with the sine, the custom modulated saw for frequency mod, and the bell envelope for amplitude mod
    std::shared_ptr<neato::ISampleSource> signal = std::make_shared<neato::ModulatedSignal>(carrier_temp, center_freq_saw_modulator, bell_envelope);
    return signal;
}

std::shared_ptr<neato::ISampleSource> CreateFlute(double center_freq, double sample_rate)
{
    std::vector<double> frequency_multiples = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    std::vector<double> gains_in_db = {-8.0, -10.0, -12.0, -19.0, -21.0, -30.0, -40.0, -42.0};
    
    
    //make composite signal
    std::shared_ptr<neato::ISampleSource> raw_sig = neato::dbCreateCompositeConstSineSignalWithCenterFreq(center_freq, std::move(frequency_multiples), std::move(gains_in_db), sample_rate);
    //make overall envelope
    std::shared_ptr<neato::ISampleSource> env_temp = neato::CreateEnvelope(neato::EnvelopeID::Bell1, sample_rate, 1.0);
    
    //make a modulated signal
    return std::make_shared<neato::ModulatedSignal>(raw_sig, std::shared_ptr<neato::ISampleSource>(), env_temp);
}

std::shared_ptr<neato::ISampleSource> CreateCompositeSignalWithBellEnvelopes(double center_freq, const neato::audio_stream_description_t& stream_desc_in)
{
    std::vector<double> frequency_multiples = {1.0, 1.272, 1.554};//, 6.0 / 3.89};
    std::vector<double> frequencies = neato::FrequenciesFromMultiples(center_freq, std::move(frequency_multiples));// = {400.0, 500.0, 600.00};
    std::vector<std::shared_ptr<neato::ISampleSource>> sine_waves = neato::CreateConstSineSignalsFromFrequencies(frequencies, stream_desc_in.sample_rate);
    
    const std::vector<double>::size_type signal_count = frequencies.size();
    std::vector<double> gains;
    gains.reserve(signal_count);
    for (std::vector<double>::size_type i = 0; i < signal_count; i++)
    {
        gains.push_back( 1.0 / (signal_count +1) );
    }
    
    //make the envelopes
    std::vector<std::shared_ptr<neato::ISampleSource>> envelopes;
    envelopes.reserve(signal_count);
    for (std::vector<double>::size_type i = 0; i < signal_count; i++)
    {
        std::shared_ptr<neato::ISampleSource> env_temp = neato::CreateEnvelope(neato::EnvelopeID::Bell1, stream_desc_in.sample_rate, gains.at(i));
        envelopes.push_back(env_temp);
    }
    
    std::shared_ptr<neato::ISampleSource> composite_signal = neato::CreateCompositeSignalWithSignalsAndEnvelopes(sine_waves, envelopes, stream_desc_in.sample_rate);
    return composite_signal;
}

std::shared_ptr<neato::ISampleSource> CreateAdditiveBell(double center_freq, const neato::audio_stream_description_t& stream_desc_in)
{
    std::vector<double> frequency_multiples = {0.56, 0.92, 1.19, 1.71, 2, 2.74, 3, 3.76, 4.07, 5.50};
    std::vector<double> frequencies = neato::FrequenciesFromMultiples(center_freq, std::move(frequency_multiples));
    std::vector<std::shared_ptr<neato::ISampleSource>> sine_waves = neato::CreateConstSineSignalsFromFrequencies(frequencies, stream_desc_in.sample_rate);
    const std::vector<double>::size_type signal_count = frequency_multiples.size();
    
    // make the envelope scale values
    constexpr double fundamental_gain = 0.5;
    std::vector<double> gains;
    gains.reserve(signal_count);
    gains.push_back(0.2);
    gains.push_back(0.5);
    gains.push_back(0.3);
    for (std::vector<double>::size_type i = 3; i < signal_count; i++)
    {
        gains.push_back( fundamental_gain / std::pow((double)1.75, (double)i) );
    }
    
    //make the envelopes
    std::vector<std::shared_ptr<neato::ISampleSource>> envelopes;
    envelopes.reserve(signal_count);
    for (auto gain : gains)
    {
        std::shared_ptr<neato::ISampleSource> env_temp = neato::CreateEnvelope(neato::EnvelopeID::Bell1, stream_desc_in.sample_rate, gain);
        envelopes.push_back(env_temp);
    }
    
    //make the composite signal
    std::shared_ptr<neato::ISampleSource> composite_signal = neato::CreateCompositeSignalWithSignalsAndEnvelopes(sine_waves, envelopes, stream_desc_in.sample_rate);
    return composite_signal;
}

std::shared_ptr<neato::ISampleSource> CreateHarmonicBells(double center_freq, const neato::audio_stream_description_t& stream_desc_in)
{
    std::shared_ptr<neato::ISampleSource> bell1 = CreateAdditiveBell(center_freq, stream_desc_in);
    std::shared_ptr<neato::ISampleSource> bell2 = CreateAdditiveBell(center_freq * 1.554, stream_desc_in);
    std::shared_ptr<neato::ISampleSource> bell3 = CreateAdditiveBell(center_freq * 2.0, stream_desc_in);
    std::shared_ptr<neato::ISampleSource> bell4 = CreateAdditiveBell(center_freq * 4.0, stream_desc_in);
    std::vector<std::shared_ptr<neato::ISampleSource>> signals = {bell1, bell2, bell3, bell4};
    std::shared_ptr<neato::ISampleSource> composite_signal = std::make_shared<neato::SampleSummer>(signals);
    return composite_signal;
}

TestRenderer::TestRenderer(const neato::audio_stream_description_t& stream_desc_in)
{
    double center_freq = 200.0f;
    //signal = CreateFMBell(center_freq, stream_desc_in);
    //signal = CreateAdditiveBell(center_freq, stream_desc_in);
    //signal = CreateHarmonicBells(center_freq, stream_desc_in);
    //signal = CreateCompositeSignalWithBellEnvelopes(center_freq, stream_desc_in);
    signal = CreateFlute(center_freq, stream_desc_in.sample_rate);
}

std::shared_ptr<neato::IRenderReturn> TestRenderer::Render(neato::render_params_t params)
{
    std::shared_ptr<neato::IRenderReturn> error = neato::CreateRenderReturn();

    SInt16 *left = (SInt16 *)params.ioData->mBuffers[0].mData;
    
    for (UInt32 frame = 0; frame < params.inNumberFrames; ++frame)
    {
        left[frame] = (SInt16)(signal->Sample() * neato::PCM_Normalize * 0.3);
    }

    // Copy left channel to right channel
    memcpy(params.ioData->mBuffers[1].mData, left, params.ioData->mBuffers[1].mDataByteSize);
    
    return error;
}
