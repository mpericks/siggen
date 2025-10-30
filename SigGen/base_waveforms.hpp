//
//  base_waveforms.hpp
//  SigGen
//
//  Created by Mike Erickson on 11/14/22.
//

#pragma once

#include <iostream>
#include <cmath>
#include <memory>
#include <numbers>
#include <map>
#include <vector>
#include <random>

constexpr static const double two_pi = std::numbers::pi * 2.0;

namespace neato
{
    class ISampleSource
    {
    public:
        virtual double Sample() = 0;
        virtual ~ISampleSource() = 0;
    };

    typedef std::vector<std::shared_ptr<neato::ISampleSource>> sample_source_vector_t;
    
    class AudioRadians : public ISampleSource
    {
    public:
        AudioRadians(double frequency_in, double sample_rate_in, std::shared_ptr<ISampleSource> frequency_modulator_in)
        : value(0.0f)
        , sample_rate(sample_rate_in)
        , frequency_modulator(frequency_modulator_in)
        {
            setFrequency(frequency_in);
        }
        AudioRadians() = delete;
        virtual double Sample()
        {
            double ret_value = value;
            if(frequency_modulator)
            {
                setFrequency(frequency_modulator->Sample());
            }
            value += increment;
            if (value > two_pi)
            {
                value -= two_pi;
            }
            return ret_value;
        }
        virtual double getFrequency() {return frequency;}
        virtual void setFrequency(double new_frequency)
        {
            frequency = new_frequency;
            increment = two_pi * new_frequency / sample_rate;
        }
        double Value() const {return value;}
    private:
        double value;
        double increment;
        double frequency;
        std::shared_ptr<ISampleSource> frequency_modulator;
        const double sample_rate;
    };

    class MutableSine : public ISampleSource
    {
    public:
        MutableSine(double frequency_in, double sample_rate_in, std::shared_ptr<ISampleSource> frequency_modulator_in)
        : theta(frequency_in, sample_rate_in, frequency_modulator_in), value(0.0f)
        {
            
        }
        virtual double Sample()
        {
            double ret_value = value;
            value = std::sin(theta.Sample());
            return ret_value;
        }
        double Value() const { return value;}
        virtual double getFrequency() {return theta.getFrequency();}
        virtual void setFrequency(double new_frequency)
        {
            theta.setFrequency(new_frequency);
        }
    private:
        AudioRadians theta;
        double value;
    };

    class ConstSine : public ISampleSource
    {
    public:
        ConstSine(double frequency_in, double sample_rate_in) : index(0)
        {
            uint32_t samples_per_cycle = uint32_t (sample_rate_in / frequency_in);// + 1;
            sine_table.reserve(samples_per_cycle);
            AudioRadians theta(frequency_in, sample_rate_in, std::shared_ptr<ISampleSource>());
            for (uint32_t i = 0; i < samples_per_cycle; i++)
            {
                sine_table.push_back(std::sin(theta.Sample()));
            }
        }
        double Sample()
        {
            double value = sine_table[index];
            index += 1;
            if (index >= sine_table.size())
            {
                index = 0;
            }
            return value;
        }
        double Value() const { return sine_table[index];}

    private:
        std::vector<double> sine_table;
        std::vector<double>::size_type index;
    };

    class ConstSaw : public ISampleSource
    {
    public:
        ConstSaw(double frequency_in, double sample_rate_in, bool negative_slope_in)
            :index(0)
        {
            uint32_t samples_per_cycle = uint32_t (sample_rate_in / frequency_in);
            saw_table.reserve(samples_per_cycle);
            AudioRadians theta(frequency_in, sample_rate_in, std::shared_ptr<ISampleSource>());
            for (uint32_t i = 0; i < samples_per_cycle; i++)
            {
                if (negative_slope_in)
                {
                    saw_table.push_back(1.0 - (2.0 * (theta.Sample() / two_pi)));
                }
                else
                {
                    saw_table.push_back((2.0 * (theta.Sample() / two_pi)) - 1.0);
                }
            }
        }
        double Sample()
        {
            double value = saw_table[index];
            index += 1;
            if (index >= saw_table.size())
            {
                index = 0;
            }
            return value;
        }
        double Value() const { return saw_table[index];}
    private:
        std::vector<double> saw_table;
        std::vector<double>::size_type index;
    };

    class MutableSaw : public ISampleSource
    {
    public:
        MutableSaw(double frequency_in, double sample_rate_in, bool negative_slope_in, std::shared_ptr<ISampleSource> frequency_modulator_in)
        : theta(frequency_in, sample_rate_in, frequency_modulator_in)
        , value(0.0f)
        , negative_slope(negative_slope_in)
        {
            
        }
        virtual double Sample()
        {
            double ret_value = value;
            if (negative_slope)
            {
                value = 1.0 - (2.0 * (theta.Sample() / two_pi));
            }
            else
            {
                value = (2.0 * (theta.Sample() / two_pi)) - 1.0;
            }
            return ret_value;
        }
        virtual double getFreguency() {return theta.getFrequency();}
        virtual void setFrequency(double new_frequency)
        {
            theta.setFrequency(new_frequency);
        }
    private:
        AudioRadians theta;
        double value;
        bool negative_slope;
    };

    class WhiteNoise : public ISampleSource
    {
    public:
        WhiteNoise()
        : random_engine(random_device())
        , random_dist(-1, 1)// Choose a random mean between -1 and 1
        {
            
        }
        virtual double Sample()
        {
            return random_dist(random_engine);
        }
    private:
        std::random_device random_device;
        std::default_random_engine random_engine;
        std::uniform_real_distribution<double> random_dist;
    };
    
    class DCOffset : public ISampleSource
    {
    public:
        DCOffset(double value_in) : value(value_in){}
        virtual double Sample()
        {
            return value;
        }
    private:
        double value;
    };

    class SampleSummer : public ISampleSource
    {
    public:
        SampleSummer(const std::vector<std::shared_ptr<ISampleSource>>& sample_sources_in) : sample_sources(sample_sources_in)
        {
            
        }
        virtual double Sample()
        {
            double ret_val = 0;
            std::for_each(sample_sources.begin(), sample_sources.end(), [&ret_val](std::shared_ptr<ISampleSource>& sampler)
            {
                ret_val += sampler->Sample();
            });
            return ret_val;
        }
    private:
        std::vector<std::shared_ptr<ISampleSource>> sample_sources;
    };

    class MutableSummer : public ISampleSource
    {
    public:
        MutableSummer() = default;
        MutableSummer(const std::vector<std::shared_ptr<ISampleSource>>& sample_sources_in) : sample_sources(sample_sources_in)
        {

        }
        MutableSummer(std::vector<std::shared_ptr<ISampleSource>>&& sample_sources_in) : sample_sources(std::move(sample_sources_in))
        {

        }
        void AddSource(std::shared_ptr<ISampleSource> source)
        {
            sample_sources.push_back(source);
        }
        void ClearSources()
        {
            sample_sources.clear();
        }
        void RemoveSource(std::shared_ptr<ISampleSource> source)
        {
            auto it = std::find(sample_sources.begin(), sample_sources.end(), source);
            if (it != sample_sources.end())
            {
                sample_sources.erase(it);
            }
        }
        uint32_t SourceCount() const
        {
            return static_cast<uint32_t>(sample_sources.size());
        }
        bool HasSource(std::shared_ptr<ISampleSource> source) const
        {
            auto it = std::find(sample_sources.begin(), sample_sources.end(), source);
            return (it != sample_sources.end());
        }
        virtual double Sample()
        {
            double ret_val = 0;
            std::for_each(sample_sources.begin(), sample_sources.end(), [&ret_val](std::shared_ptr<ISampleSource>& sampler)
            {
                ret_val += sampler->Sample();
            });
            return ret_val;
        }
    private:
        std::vector<std::shared_ptr<ISampleSource>> sample_sources;
    };
    
    class SampleMultiplier : public ISampleSource
    {
    public:
        SampleMultiplier(std::shared_ptr<ISampleSource> source1_in, std::shared_ptr<ISampleSource> source2_in)
        : source1(source1_in)
        , source2(source2_in)
        {
            
        }
        SampleMultiplier(std::shared_ptr<ISampleSource> source1_in, double multiplier)
        : source1(source1_in)
        {
            source2 = std::make_shared<DCOffset>(multiplier);
        }
        virtual double Sample()
        {
            return source1->Sample() * source2->Sample();
        }
    private:
        std::shared_ptr<ISampleSource> source1;
        std::shared_ptr<ISampleSource> source2;
    };
    
    std::vector<double> FrequenciesFromMultiples(double center_freq, std::vector<double>&& frequency_multiples);
    std::vector<std::shared_ptr<ISampleSource>> CreateConstSineArray(std::vector<double> frequencies, double sample_rate);
    std::vector<std::shared_ptr<ISampleSource>> CreateDCOffsetArray(std::vector<double> offsets);
    std::vector<std::shared_ptr<ISampleSource>> CreateMultiplierArray(std::vector<std::shared_ptr<ISampleSource>> source1_array, std::vector<double> multipliers);
    std::vector<std::shared_ptr<ISampleSource>> CreateMultiplierArray(std::vector<std::shared_ptr<ISampleSource>> source1_array, std::vector<std::shared_ptr<ISampleSource>> source2_array);
};

