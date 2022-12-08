//
//  envelope.cpp
//  SigGen
//
//  Created by Mike Erickson on 12/4/22.
//

#include "envelope.hpp"
#include "audio_time.h"
#include <vector>
namespace neato
{
enum class GainSegmentId
{
    attack,
    decay
};
}

class LinearEnvelopeSegment : public neato::IEnvelopeSegment
{
public:
    LinearEnvelopeSegment(float sample_rate_in, float gain_start_value, float gain_target_value, float gain_duration_time, neato::GainSegmentId id)
        : sample_time_accumulator(sample_rate_in)
        , current_segment_sample_index(0)
        , p_callback(nullptr)
        , id(id)
    {
        //how many samples in duration?
        uint32_t samples_per_duration = (uint32_t)(sample_rate_in * gain_duration_time);
        
        //how much gain is added during the entire duration?
        double gain_per_duration = (double)gain_target_value - (double)gain_start_value;
        
        //how much gain is added for each sample?
        //now you can save gain added per sample and do that each increment
        // or you could precalculate the whole series
        // and just look up next value each increment
        double gain_per_sample = gain_per_duration / samples_per_duration;
        gains_for_each_sample.reserve(samples_per_duration);
        double gain_now = gain_start_value;
        for (std::vector<double>::size_type samp_idx  = 0; samp_idx < samples_per_duration; samp_idx++)
        {
            gains_for_each_sample.push_back(gain_now);
            gain_now += gain_per_sample;
        }
    }
    virtual float Increment()
    {
        float return_gain = gains_for_each_sample[current_segment_sample_index];
        current_segment_sample_index += 1;
        if (current_segment_sample_index >= gains_for_each_sample.size())
        {
            current_segment_sample_index = 0;
            if (nullptr != p_callback)
            {
                p_callback->StateComplete((int)id);
            }
        }
        
        return return_gain;
    }
    virtual void SetGainStateCompletionCallback(std::shared_ptr<neato::IStateCompletionCallback> callback_in)
    {
        callback = callback_in;
        p_callback = callback.get();
    }
    virtual void SetGainStateCompletionCallback(neato::IStateCompletionCallback* p_callback_in)
    {
        p_callback = p_callback_in;
    }
private:
    neato::AudioTime sample_time_accumulator;
    
    std::vector<double> gains_for_each_sample;
    std::vector<double>::size_type current_segment_sample_index;
    std::shared_ptr<neato::IStateCompletionCallback> callback;
    neato::IStateCompletionCallback* p_callback;
    neato::GainSegmentId id;
};

class Bell1Envelope : public neato::IEnvelope, public neato::IStateCompletionCallback
{
public:
    Bell1Envelope(float sample_rate_in)
        : attack(sample_rate_in, 0.0, 0.5, 0.003, neato::GainSegmentId::attack)
        , decay(sample_rate_in, 0.5, 0.0, 3.0, neato::GainSegmentId::decay)
        , current_segment(nullptr)
    {
        attack.SetGainStateCompletionCallback(this);
        decay.SetGainStateCompletionCallback(this);
        current_segment = &attack;
    }
    virtual float Increment()
    {
        float gain = 0.0;
        if (nullptr != current_segment)
        {
            gain = current_segment->Increment();
        }
        else
        {
            assert(false);
        }
        return gain;
    }
    virtual void StateComplete(int stage_id)
    {
        if (stage_id == (int)neato::GainSegmentId::attack)
        {
            current_segment = &decay;
        }
        if (stage_id == (int)neato::GainSegmentId::decay)
        {
            current_segment = &attack;
        }
    }
private:
    LinearEnvelopeSegment attack;
    LinearEnvelopeSegment decay;
    neato::IEnvelopeSegment* current_segment;
    
};

std::shared_ptr<neato::IEnvelope> CreateBell1(float sample_rate_in)
{
    std::shared_ptr<neato::IEnvelope> envelope = std::make_shared<Bell1Envelope>(sample_rate_in);
    
    return envelope;
}

std::shared_ptr<neato::IEnvelope> neato::CreateEnvelope(neato::EnvelopeID id, float sample_rate_in)
{
    std::shared_ptr<neato::IEnvelope> envelope;
    
    switch (id)
    {
        case neato::EnvelopeID::Bell1:
            envelope = CreateBell1(sample_rate_in);
            break;
        default:
            break;
    }
    
    return envelope;
}
