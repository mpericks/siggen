//
//  envelope.hpp
//  SigGen
//
//  Created by Mike Erickson on 12/4/22.
//

#pragma once
#include "base_waveforms.hpp"
#include <memory>

namespace neato
{
    class IStateCompletionCallback
    {
    public:
        virtual void StateComplete(int stage_id)=0;
    };

    class IEnvelopeSegment : public neato::ISampleSource
    {
    public:
        virtual void SetGainStateCompletionCallback(std::shared_ptr<IStateCompletionCallback> callback_in)=0;
        virtual void SetGainStateCompletionCallback(IStateCompletionCallback* p_callback_in)=0;
    };

    enum class EnvelopeID
    {
        Bell1
    };

    std::shared_ptr<neato::ISampleSource> CreateConstantGain(double gain);
    std::shared_ptr<neato::ISampleSource> CreateEnvelope(EnvelopeID id, double sample_rate_in, double scale);

    double dbToGain(double db);
    std::vector<double> dbToGains(std::vector<double>&& gains_in_db);
};

