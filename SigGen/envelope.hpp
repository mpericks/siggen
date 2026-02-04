//
//  envelope.hpp
//  SigGen
//
//  Created by Mike Erickson on 12/4/22.
//

#pragma once
#include "base_waveforms.hpp"

namespace Neato
{
    class IStateCompletionCallback
    {
    public:
        virtual void StateComplete(int stage_id)=0;
    };

    class IEnvelopeSegment : public Neato::ISampleSource
    {
    public:
        virtual void SetGainStateCompletionCallback(std::shared_ptr<IStateCompletionCallback> callback_in)=0;
        virtual void SetGainStateCompletionCallback(IStateCompletionCallback* p_callback_in)=0;
    };

    enum class EnvelopeID
    {
        Bell1
    };

    std::shared_ptr<Neato::ISampleSource> CreateEnvelope(EnvelopeID id, double sample_rate_in, double scale);

    double dbToGain(double db);
    std::vector<double> dbToGains(std::vector<double>&& gains_in_db);
};

