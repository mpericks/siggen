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
    //frequency of the carrier gets modulated by a saw with a constant gain
    std::shared_ptr<neato::ISampleSource> saw_temp = std::make_shared<neato::ConstSaw>(1.4f * center_freq, stream_desc_in.sample_rate, false);
    
    //make a modualted signal with the saw and the gain
    std::shared_ptr<neato::ISampleSource> saw_with_gain = std::make_shared<neato::SampleMultiplier>(saw_temp, 160.0);
    
    //make a frequency modulator
    //std::shared_ptr<neato::ICustomModulatorFunction> center_freq_mod = std::make_shared<neato::CenterFrequencyModulator>(center_freq);
    std::vector<std::shared_ptr<neato::ISampleSource>> frequency_modulator_signals = {saw_with_gain, std::make_shared<neato::DCOffset>(center_freq)};
    std::shared_ptr<neato::ISampleSource> frequncy_modulator = std::make_shared<neato::SampleSummer>(frequency_modulator_signals);
    
    //create the sine wave with the frequency modulator
    std::shared_ptr<neato::ISampleSource> carrier_temp = std::make_shared<neato::MutableSine>(center_freq, stream_desc_in.sample_rate, frequncy_modulator);
    
    //make an envelope for the bell
    std::shared_ptr<neato::ISampleSource> bell_envelope = neato::CreateEnvelope(neato::EnvelopeID::Bell1, stream_desc_in.sample_rate, 1.0);
    
    //make an overall modulated signal with the sine, the custom modulated saw for frequency mod, and the bell envelope for amplitude mod
    std::shared_ptr<neato::ISampleSource> signal = std::make_shared<neato::SampleMultiplier>(carrier_temp, bell_envelope);
    return signal;
}

std::shared_ptr<neato::ISampleSource> CreateFlute(double center_freq, double sample_rate)
{
    const uint8_t harmonic_count = 6;
    const double tremolo_freq = 5.0;
    const double white_noise_gain_db = -36.0;
    std::vector<double> frequency_multiples = {1.0, 2.00, 3.0, 4.0, 5.0, 6.0};
    std::vector<double> frequency_gains_in_db = {-7.5, -11.0, -13.0, -19.0, -30.0, -42.0};
    std::vector<double> tremolo_gains = { 0.1001,  0.2, 0.1, 0.001, 0.001, 0.001 };
        
    //tremolo modulators for higher harmonics
    std::vector<std::shared_ptr<neato::ISampleSource>> tremolo_sines;
    tremolo_sines.reserve(harmonic_count);
    for(uint32_t i = 0; i < harmonic_count; i++)
    {
        tremolo_sines.push_back(std::make_shared<neato::ConstSine>(tremolo_freq, sample_rate));
    }
    
    //apply a gain to the tremolos. Don't want a huge variation in volume
    std::vector<std::shared_ptr<neato::ISampleSource>> tremolos_with_gain = neato::CreateMultiplierArray(tremolo_sines, tremolo_gains);

    //create clean sine waves
    std::vector<double> frequencies = neato::FrequenciesFromMultiples(center_freq, std::move(frequency_multiples));
    std::vector<std::shared_ptr<neato::ISampleSource>> signals = neato::CreateConstSineArray(frequencies, sample_rate);
    
    //put tremolo modulators on sines
    std::vector<std::shared_ptr<neato::ISampleSource>> signals_with_tremolo;
    signals_with_tremolo.reserve(harmonic_count);
    for(uint32_t i = 0; i < harmonic_count; i++)
    {
        std::vector<std::shared_ptr<neato::ISampleSource>> signals_to_sum;
        signals_to_sum.push_back(signals.at(i));
        signals_to_sum.push_back(tremolos_with_gain.at(i));
        signals_with_tremolo.push_back(std::make_shared<neato::SampleSummer>(signals_to_sum));
    }
    
    std::vector<double> gain_values = neato::dbToGains(std::move(frequency_gains_in_db));
    //gain multipliers
    std::vector<std::shared_ptr<neato::ISampleSource>> signals_with_tremolo_and_gain = neato::CreateMultiplierArray(signals_with_tremolo, gain_values);

    //add noise signal
    std::shared_ptr<neato::ISampleSource> noise = std::make_shared<neato::WhiteNoise>();
    std::shared_ptr<neato::ISampleSource> noise_with_gain = std::make_shared<neato::SampleMultiplier>(noise, neato::dbToGain(white_noise_gain_db));
    signals_with_tremolo_and_gain.push_back(noise_with_gain);
    
    //make summed signal
    std::shared_ptr<neato::ISampleSource> raw_sig = std::make_shared<neato::SampleSummer>(signals_with_tremolo_and_gain);
    
    //make overall envelope
    std::shared_ptr<neato::ISampleSource> env_temp = neato::CreateEnvelope(neato::EnvelopeID::Bell1, sample_rate, 1.0);
    
    //make a modulated signal
    return std::make_shared<neato::SampleMultiplier>(raw_sig, env_temp);
}

std::shared_ptr<neato::ISampleSource> CreateCompositeSignalWithBellEnvelopes(double center_freq, const neato::audio_stream_description_t& stream_desc_in)
{
    std::vector<double> frequency_multiples = {1.0, 1.272, 1.554};//, 6.0 / 3.89};
    std::vector<double> frequencies = neato::FrequenciesFromMultiples(center_freq, std::move(frequency_multiples));// = {400.0, 500.0, 600.00};
    std::vector<std::shared_ptr<neato::ISampleSource>> sine_waves = neato::CreateConstSineArray(frequencies, stream_desc_in.sample_rate);
    
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
    
    std::shared_ptr<neato::ISampleSource> composite_signal = std::make_shared<neato::SampleSummer>(neato::CreateMultiplierArray(sine_waves, envelopes));
    return composite_signal;
}

std::shared_ptr<neato::ISampleSource> CreateAdditiveBell(double center_freq, const neato::audio_stream_description_t& stream_desc_in)
{
    std::vector<double> frequency_multiples = {0.56, 0.92, 1.19, 1.71, 2, 2.74, 3, 3.76, 4.07, 5.50};
    std::vector<double> frequencies = neato::FrequenciesFromMultiples(center_freq, std::move(frequency_multiples));
    std::vector<std::shared_ptr<neato::ISampleSource>> sine_waves = neato::CreateConstSineArray(frequencies, stream_desc_in.sample_rate);
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
    
    //multiply envelopes and signals
    std::vector<std::shared_ptr<neato::ISampleSource>> multiplied_signals = neato::CreateMultiplierArray(sine_waves, envelopes);
    
    //sum all the signals
    std::shared_ptr<neato::ISampleSource> composite_signal = std::make_shared<neato::SampleSummer>(multiplied_signals);
    
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
    double center_freq = 300.0f;
    //signal = CreateFMBell(center_freq, stream_desc_in);
    //signal = CreateAdditiveBell(center_freq, stream_desc_in);
    //signal = CreateHarmonicBells(center_freq, stream_desc_in);
    signal = CreateCompositeSignalWithBellEnvelopes(center_freq, stream_desc_in);
    //signal = CreateFlute(center_freq, stream_desc_in.sample_rate);
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
