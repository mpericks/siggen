//
//  audio_time.h
//  SigGen
//
//  Created by Mike Erickson on 12/4/22.
//

#pragma once

namespace neato
{
    class AudioTime
    {
    public:
        AudioTime(double sample_rate_in)
            : sample_rate(sample_rate_in)
            , accumulated_samples(0.0)
            , seconds(0.0)
            , seconds_per_sample(1.0 / sample_rate_in)
        {
            
        }
        AudioTime() = delete;
        double Increment()
        {
            accumulated_samples += 1.0;
            seconds = accumulated_samples / sample_rate;
            return seconds;
        }
        double Value() const
        {
            return seconds;
        }
        void Reset()
        {
            accumulated_samples = 0;
        }
    private:
        double sample_rate;
        double accumulated_samples;
        double seconds;
        double seconds_per_sample;
    };
};



