//
//  composite_waveforms.hpp
//  SigGen
//
//  Created by Mike Erickson on 1/8/23.
//

#pragma once

#include <memory>
#include <vector>
#include "base_waveforms.hpp"

namespace neato
{
    std::vector<double> FrequenciesFromMultiples(double center_freq, std::vector<double>&& frequency_multiples);
    //std::vector<std::shared_ptr<ISampleSource>> CreateConstSineSignalsFromFrequencies(std::vector<double> frequencies, double sample_rate);
    std::shared_ptr<neato::ISampleSource> CreateCompositeConstSineSignal(std::vector<double> frequencies, std::vector<double> gains, double sample_rate);
    std::shared_ptr<neato::ISampleSource> CreateCompositeSignalWithSignalsAndEnvelopes(std::vector<std::shared_ptr<neato::ISampleSource>> waveforms, std::vector<std::shared_ptr<neato::ISampleSource>> envelopes, double sample_rate);
    std::shared_ptr<neato::ISampleSource> dbCreateCompositeConstSineSignalWithCenterFreq(double center_freq, std::vector<double>&& frequency_multiples, std::vector<double>&& gains_in_db, double sample_rate);
};

