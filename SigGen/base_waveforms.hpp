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

constexpr static const float two_pi = std::numbers::pi * 2.0f;

namespace neato
{
    class IMutableFrequency
    {
    public:
        virtual double getFrequency() = 0;
        virtual void setFrequency(double new_frequency) = 0;
    };

    class ISampleSource
    {
    public:
        virtual double Sample() = 0;
        virtual ~ISampleSource() = 0;
        virtual IMutableFrequency* GetMutableFrequencyPtr(){return nullptr;}
    };
    
    class AudioRadians : public ISampleSource, IMutableFrequency
    {
    public:
        AudioRadians(float frequency_in, float sample_rate_in) : value(0.0f), sample_rate(sample_rate_in)
        {
            setFrequency(frequency_in);
        }
        AudioRadians() = delete;
        virtual double Sample()
        {
            double ret_value = value;
            value += increment;
            if (value > two_pi)
            {
                value -= two_pi;
            }
            return ret_value;
        }
        virtual IMutableFrequency* GetMutableFrequencyPtr()
        {
            return this;
        }
        virtual double getFrequency() {return frequency;}
        virtual void setFrequency(double new_frequency)
        {
            frequency = new_frequency;
            increment = two_pi * new_frequency / sample_rate;
        }
        float Value() const {return value;}
    private:
        double value;
        double increment;
        double frequency;
        const float sample_rate;
    };

    class MutableSine : public ISampleSource, IMutableFrequency
    {
    public:
        MutableSine(float frequency_in, float sample_rate_in) : theta(frequency_in, sample_rate_in), value(0.0f)
        {
            
        }
        virtual double Sample()
        {
            double ret_value = value;
            value = std::sinf(theta.Sample());
            return ret_value;
        }
        float Value() const { return value;}
        virtual IMutableFrequency* GetMutableFrequencyPtr()
        {
            return this;
        }
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
        ConstSine(float frequency_in, float sample_rate_in) : index(0)
        {
            uint32_t samples_per_cycle = uint32_t (sample_rate_in / frequency_in);// + 1;
            sine_table.reserve(samples_per_cycle);
            AudioRadians theta(frequency_in, sample_rate_in);
            for (uint32_t i = 0; i < samples_per_cycle; i++)
            {
                sine_table.push_back(std::sinf(theta.Sample()));
            }
        }
        double Sample()
        {
            float value = sine_table[index];
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
        ConstSaw(float frequency_in, float sample_rate_in, bool negative_slope_in)
        {
            uint32_t samples_per_cycle = uint32_t (sample_rate_in / frequency_in);// + 1;
            saw_table.reserve(samples_per_cycle);
            AudioRadians theta(frequency_in, sample_rate_in);
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
        float Value() const { return saw_table[index];}
    private:
        std::vector<double> saw_table;
        std::vector<double>::size_type index;
    };

    class MutableSaw : public ISampleSource, IMutableFrequency
    {
    public:
        MutableSaw(float frequency_in, float sample_rate_in, bool negative_slope_in) : theta(frequency_in, sample_rate_in), value(0.0f), negative_slope(negative_slope_in)
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
        virtual IMutableFrequency* GetMutableFrequencyPtr()
        {
            return this;
        }
        virtual double getFreguency() {return theta.getFrequency();}
        virtual void setFrequency(float new_frequency)
        {
            theta.setFrequency(new_frequency);
        }
    private:
        AudioRadians theta;
        double value;
        bool negative_slope;
    };

    class ICustomModulatorFunction
    {
    public:
        virtual double Modulate(double modulator_value) = 0;
    };
    
    class CenterFrequencyModulator : public ICustomModulatorFunction
    {
    public:
        CenterFrequencyModulator(double center_frequency_in) : center_frequency(center_frequency_in){}
        virtual double Modulate(double signal_in)
        {
            return center_frequency + signal_in;
        }
    private:
        double center_frequency;
    };

    class CustomModulator : public ISampleSource
    {
    public:
        CustomModulator(std::shared_ptr<ISampleSource> modulation_signal_in, std::shared_ptr<ICustomModulatorFunction> modulation_function_in)
        : modulation_signal(modulation_signal_in)
        , modulation_function(modulation_function_in)
        {
            
        }
        virtual double Sample()
        {
            return modulation_function->Modulate(modulation_signal->Sample());
        }
    private:
        std::shared_ptr<ISampleSource> modulation_signal;
        std::shared_ptr<ICustomModulatorFunction> modulation_function;
    };

    class ModulatedSignal : public neato::ISampleSource
    {
    public:
        ModulatedSignal(std::shared_ptr<ISampleSource> carrier_in, std::shared_ptr<ISampleSource> frequency_modulator_in, std::shared_ptr<ISampleSource> amplitude_modulator_in)
        : carrier(carrier_in)
        , frequency_modulator(frequency_modulator_in)
        , amplitude_modulator(amplitude_modulator_in)
        {
            
        }
        virtual double Sample()
        {
            double ret_value = carrier->Sample();
            if (amplitude_modulator)
            {
                ret_value *= amplitude_modulator->Sample();
            }
            neato::IMutableFrequency* change_freq = carrier->GetMutableFrequencyPtr();
            if ( nullptr != change_freq )
            {
                if (frequency_modulator)
                {
                    change_freq->setFrequency(frequency_modulator->Sample());
                }
            }
            return ret_value;
        }
    private:
        std::shared_ptr<ISampleSource> carrier;
        std::shared_ptr<ISampleSource> frequency_modulator;
        std::shared_ptr<ISampleSource> amplitude_modulator;
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
};

