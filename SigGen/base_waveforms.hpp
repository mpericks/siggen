//
//  Sine.hpp
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
    class AudioRadians
    {
    public:
        AudioRadians(float frequency_in, float sample_rate_in) : value(0.0f), frequency(frequency_in), sample_rate(sample_rate_in)
        {}
        AudioRadians() = delete;
        double Increment()
        {
            value += two_pi * frequency / sample_rate;
            if (value > two_pi)
            {
                value -= two_pi;
            }
            return value;
        }
        void setFrequency(float new_frequency){frequency = new_frequency;}
        float Value() const {return value;}
    private:
        float value;
        float frequency;
        const float sample_rate;
    };

    class MutableSine
    {
    public:
        MutableSine(float frequency_in, float sample_rate_in) : theta(frequency_in, sample_rate_in), value(0.0f)
        {
            
        }
        float Increment()
        {
            value = std::sinf(theta.Increment());
            return value;
        }
        float Value() const { return value;}
        void setFrequency(float new_frequency)
        {
            theta.setFrequency(new_frequency);
        }
    private:
        AudioRadians theta;
        float value;
    };

    class Sine
    {
    public:
        Sine(float frequency_in, float sample_rate_in) : index(0)
        {
            uint32_t samples_per_cycle = uint32_t (sample_rate_in / frequency_in) + 1;
            sine_table.reserve(samples_per_cycle);
            AudioRadians theta(frequency_in, sample_rate_in);
            for (uint32_t i = 0; i < samples_per_cycle; i++)
            {
                sine_table[i] = theta.Value();
                theta.Increment();
            }
        }
        float Increment()
        {
            float value = sine_table[index];
            index += 1;
            if (index >= sine_table.size())
            {
                index = 0;
            }
            return value;
        }
        float Value() const { return sine_table[index];}

    private:
        std::vector<float> sine_table;
        std::vector<float>::size_type index;
    };

    class MutableSaw
    {
    public:
        MutableSaw(float frequency_in, float sample_rate_in, bool negative_slope_in) : theta(frequency_in, sample_rate_in), value(0.0f), negative_slope(negative_slope_in)
        {
            
        }
        float Increment()
        {
            if (negative_slope)
            {
                value = 1.0 - (2.0 * (theta.Increment() / two_pi));
            }
            else
            {
                value = (2.0 * (theta.Increment() / two_pi)) - 1.0;
            }
            return value;
        }
        float Value() const { return value;}
        void setFrequency(float new_frequency)
        {
            theta.setFrequency(new_frequency);
        }
    private:
        AudioRadians theta;
        float value;
        bool negative_slope;
    };
};

