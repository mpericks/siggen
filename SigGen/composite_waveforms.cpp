//
//  composite_waveforms.cpp
//  SigGen
//
//  Created by Mike Erickson on 1/8/23.
//

#include "composite_waveforms.hpp"
#include "envelope.hpp"

//std::vector<double> neato::FrequenciesFromMultiples(double center_freq, std::vector<double>&& frequency_multiples)
//{
//    std::vector<double> frequencies;
//    frequencies.reserve(frequency_multiples.size());
//    std::for_each(frequency_multiples.begin(), frequency_multiples.end(), [center_freq, &frequencies](double multiplier)
//    {
//        frequencies.push_back(center_freq * multiplier);
//    });
//    return frequencies;
//}

//std::vector<std::shared_ptr<neato::ISampleSource>> neato::CreateConstSineSignalsFromFrequencies(std::vector<double> frequencies, double sample_rate)
//{
//    const std::vector<double>::size_type signal_count = frequencies.size();
//
//    //make the const sine signals
//    std::vector<std::shared_ptr<neato::ISampleSource>> signals;
//    signals.reserve(signal_count);
//    for (std::vector<double>::size_type i = 0; i < signal_count; i++)
//    {
//        std::shared_ptr<neato::ISampleSource> carrier = std::make_shared<neato::ConstSine>(frequencies.at(i), sample_rate);
//        signals.push_back(carrier);
//    }
//    return signals;
//}
//
//std::shared_ptr<neato::ISampleSource> neato::CreateCompositeSignalWithSignalsAndEnvelopes(std::vector<std::shared_ptr<neato::ISampleSource>> waveforms, std::vector<std::shared_ptr<neato::ISampleSource>> envelopes, double sample_rate)
//{
//    const std::vector<double>::size_type signal_count = waveforms.size();
//    assert(envelopes.size() == signal_count);
//    //make the modulated signals
//    std::vector<std::shared_ptr<neato::ISampleSource>> signals;
//    signals.reserve(signal_count);
//    auto envelope_iterator = envelopes.begin();
//    std::for_each(waveforms.begin(), waveforms.end(), [&signals, &envelope_iterator](std::shared_ptr<neato::ISampleSource> waveform)
//    {
//        std::shared_ptr<neato::ISampleSource> sig_temp = std::make_shared<neato::ModulatedSignal>(waveform, std::shared_ptr<neato::ISampleSource>(), *(envelope_iterator));
//        signals.push_back(sig_temp);
//        envelope_iterator = std::next(envelope_iterator);
//    });
//    
//    //make the composite signal
//    std::shared_ptr<neato::ISampleSource> composite_signal = std::make_shared<neato::SampleSummer>(signals);
//    return composite_signal;
//}
//
//std::shared_ptr<neato::ISampleSource> neato::CreateCompositeConstSineSignal(std::vector<double> frequencies, std::vector<double> gains, double sample_rate)
//{
//    const std::vector<double>::size_type signal_count = frequencies.size();
//    assert(signal_count == gains.size());
//    
//    //make the const gain envelopes
//    std::vector<std::shared_ptr<neato::ISampleSource>> envelopes = CreateDCOffsetArray(gains);
//    
//    //make the modulated signals
//    std::vector<std::shared_ptr<neato::ISampleSource>> signals;
//    signals.reserve(signal_count);
//    for (std::vector<double>::size_type i = 0; i < signal_count; i++)
//    {
//        std::shared_ptr<neato::ISampleSource> carrier = std::make_shared<neato::ConstSine>(frequencies.at(i), sample_rate);
//        std::shared_ptr<neato::ISampleSource> sig_temp = std::make_shared<neato::ModulatedSignal>(carrier, std::shared_ptr<neato::ISampleSource>(), envelopes.at(i));
//        signals.push_back(sig_temp);
//    }
//    
//    //make the composite signal
//    std::shared_ptr<neato::ISampleSource> composite_signal = std::make_shared<neato::SampleSummer>(signals);
//    return composite_signal;
//}
//
//std::shared_ptr<neato::ISampleSource> neato::dbCreateCompositeConstSineSignalWithCenterFreq(double center_freq, std::vector<double>&& frequency_multiples, std::vector<double>&& gains_in_db, double sample_rate)
//{
//    const std::vector<double>::size_type signal_count = frequency_multiples.size();
//    assert(signal_count == gains_in_db.size());
//    
//    std::vector<double> frequencies = neato::FrequenciesFromMultiples(center_freq, std::move(frequency_multiples));
//    std::vector<double> gains = neato::dbToGains(std::move(gains_in_db));
//    
//    return neato::CreateCompositeConstSineSignal(frequencies, gains, sample_rate);
//}

