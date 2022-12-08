//
//  envelope.hpp
//  SigGen
//
//  Created by Mike Erickson on 12/4/22.
//

#pragma once
#include <memory>

namespace neato
{
    class IStateCompletionCallback
    {
    public:
        virtual void StateComplete(int stage_id)=0;
    };

    class IEnvelopeSegment
    {
    public:
        virtual float Increment() = 0;
        virtual void SetGainStateCompletionCallback(std::shared_ptr<IStateCompletionCallback> callback_in)=0;
        virtual void SetGainStateCompletionCallback(IStateCompletionCallback* p_callback_in)=0;
    };

    enum class EnvelopeID
    {
        Bell1
    };

    class IEnvelope
    {
    public:
        virtual float Increment() = 0;
    };

    std::shared_ptr<IEnvelope> CreateEnvelope(EnvelopeID id, float sample_rate_in);
};

