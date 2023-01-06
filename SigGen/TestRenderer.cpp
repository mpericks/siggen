//
//  TestRenderer.cpp
//  SigGen
//
//  Created by Mike Erickson on 10/10/22.
//

#include "TestRenderer.hpp"
#include "base_waveforms.hpp"

TestRenderer::TestRenderer(const neato::audio_stream_description_t& stream_desc_in)
{
    double center_freq = 400.0f;
    std::shared_ptr<neato::ISampleSource> carrier_temp = std::make_shared<neato::MutableSine>(center_freq, stream_desc_in.sample_rate);
    //frequency of the carrier gets modulated by a saw with a constant gain
    std::shared_ptr<neato::ISampleSource> saw_temp = std::make_shared<neato::ConstSaw>(560.0f, stream_desc_in.sample_rate, false);
    std::shared_ptr<neato::ISampleSource> mod_gain_temp = neato::CreateConstantGain(160.0f);
    
    //make a modualted signal with the saw and the gain
    std::shared_ptr<neato::ISampleSource> saw_with_gain = std::make_shared<neato::ModulatedSignal>(saw_temp, std::shared_ptr<neato::ISampleSource>(), mod_gain_temp);
    //make a center frequency modulator function
    std::shared_ptr<neato::ICustomModulatorFunction> center_freq_mod = std::make_shared<neato::CenterFrequencyModulator>(center_freq);
    //then make the modulation sample source with the saw, the custom func and the gain (no frequency modulation on the saw. It PERFORMS frequency modulation on the mutable sine)
    std::shared_ptr<neato::ISampleSource> center_freq_saw_modulator = std::make_shared<neato::CustomModulator>(saw_with_gain, center_freq_mod);
    
    //make an envelope for the bell
    std::shared_ptr<neato::ISampleSource> bell_envelope = neato::CreateEnvelope(neato::EnvelopeID::Bell1, stream_desc_in.sample_rate);
    
    //make an overall modulated signal with the custom modulated saw and the sine
    signal = std::make_shared<neato::ModulatedSignal>(carrier_temp, center_freq_saw_modulator, bell_envelope);
}

std::shared_ptr<neato::IRenderReturn> TestRenderer::Render(neato::render_params_t params)
{
    std::shared_ptr<neato::IRenderReturn> error = neato::CreateRenderReturn();

    SInt16 *left = (SInt16 *)params.ioData->mBuffers[0].mData;
    
    for (UInt32 frame = 0; frame < params.inNumberFrames; ++frame)
    {
        left[frame] = (SInt16)(signal->Sample() * neato::PCM_Normalize);
    }

    // Copy left channel to right channel
    memcpy(params.ioData->mBuffers[1].mData, left, params.ioData->mBuffers[1].mDataByteSize);
    
    return error;
}
