//
//  TestRenderer.cpp
//  SigGen
//
//  Created by Mike Erickson on 10/10/22.
//

#include "TestRenderer.hpp"
#include "base_waveforms.hpp"

TestRenderer::TestRenderer(const neato::audio_stream_description_t& stream_desc_in)
: frequency(400.0f)
, carrier(frequency, stream_desc_in.sample_rate)
, modulator(560.0f, stream_desc_in.sample_rate, false)
, _stream_desc(stream_desc_in)
, modulator_gain(190.0)
, current_frequency(frequency)
, bell_envelope(neato::CreateEnvelope(neato::EnvelopeID::Bell1, stream_desc_in.sample_rate))
{
    
}

std::shared_ptr<neato::IRenderReturn> TestRenderer::Render(neato::render_params_t params)
{
    std::shared_ptr<neato::IRenderReturn> error = neato::CreateRenderReturn();

    SInt16 *left = (SInt16 *)params.ioData->mBuffers[0].mData;
    
    for (UInt32 frame = 0; frame < params.inNumberFrames; ++frame)
    {
        left[frame] = (SInt16)(carrier.Value() * neato::PCM_Normalize * bell_envelope->Increment());
        
        current_frequency = frequency + (modulator.Increment() * modulator_gain);
        carrier.setFrequency(current_frequency);
        carrier.Increment();
    }

    // Copy left channel to right channel
    memcpy(params.ioData->mBuffers[1].mData, left, params.ioData->mBuffers[1].mDataByteSize);
    
    return error;
}
