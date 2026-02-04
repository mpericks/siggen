//
//  Sine.cpp
//  SigGen
//
//  Created by Mike Erickson on 11/14/22.
//

#include "base_waveforms.hpp"
#include <cassert>

namespace Neato
{
    ISampleSource::~ISampleSource()
    {
        
    }
    
    std::vector<double> FrequenciesFromMultiples(double center_freq, std::vector<double>&& frequency_multiples)
    {
        std::vector<double> frequencies;
        frequencies.reserve(frequency_multiples.size());
        std::for_each(frequency_multiples.begin(), frequency_multiples.end(), [center_freq, &frequencies](double multiplier)
        {
            frequencies.push_back(center_freq * multiplier);
        });
        return frequencies;
    }
    
    std::vector<std::shared_ptr<ISampleSource>> CreateMultiplierArray(std::vector<std::shared_ptr<ISampleSource>> source1_array, std::vector<std::shared_ptr<ISampleSource>> source2_array)
    {
        std::vector<std::shared_ptr<Neato::ISampleSource>> ret_array;
        std::vector<std::shared_ptr<Neato::ISampleSource>>::size_type signal_count = source1_array.size();
        assert(signal_count == source2_array.size());
        ret_array.reserve(signal_count);
        for( uint32_t i = 0; i < signal_count; i++)
        {
            ret_array.push_back(std::make_shared<Neato::SampleMultiplier>(source1_array.at(i), source2_array.at(i)));
        }
        return ret_array;
    }
    std::vector<std::shared_ptr<Neato::ISampleSource>> CreateMultiplierArray(std::vector<std::shared_ptr<ISampleSource>> source1_array, std::vector<double> multipliers)
    {
        std::vector<std::shared_ptr<Neato::ISampleSource>> ret_array;
        std::vector<std::shared_ptr<Neato::ISampleSource>>::size_type signal_count = source1_array.size();
        assert(signal_count == multipliers.size());
        ret_array.reserve(signal_count);
        for( uint32_t i = 0; i < signal_count; i++)
        {
            ret_array.push_back(std::make_shared<Neato::SampleMultiplier>(source1_array.at(i), multipliers.at(i)));
        }
        return ret_array;
    }

    std::vector<std::shared_ptr<Neato::ISampleSource>> CreateDCOffsetArray(std::vector<double> offsets)
    {
        std::vector<std::shared_ptr<Neato::ISampleSource>> ret_array;
        std::vector<double>::size_type signal_count = offsets.size();
        ret_array.reserve(signal_count);
        for( double offset : offsets)
        {
            ret_array.push_back(std::make_shared<Neato::DCOffset>(offset));
        }
        return ret_array;
    }

    std::vector<std::shared_ptr<ISampleSource>> CreateConstSineArray(std::vector<double> frequencies, double sample_rate)
    {
        const std::vector<double>::size_type signal_count = frequencies.size();
        
        //make the const sine signals
        std::vector<std::shared_ptr<Neato::ISampleSource>> signals;
        signals.reserve(signal_count);
        for (std::vector<double>::size_type i = 0; i < signal_count; i++)
        {
            std::shared_ptr<Neato::ISampleSource> carrier = std::make_shared<Neato::ConstSine>(frequencies.at(i), sample_rate);
            signals.push_back(carrier);
        }
        return signals;
    }
}
