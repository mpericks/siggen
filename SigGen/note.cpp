#include "note.h"
#include <memory>
#include <vector>

namespace neato
{
    class SequenceSampleSource : public ISampleSource
    {
    public:
        SequenceSampleSource(const std::vector<sequence_element>& elements_in, double sample_rate_in)
        : elements(elements_in)
        , summer()
        , accumulated_time(0.0)
        , sample_time(1.0 / sample_rate_in)
        {
            UpdateSummer(accumulated_time);
        }
        virtual double Sample() override
        {
            double sample_value = summer.Sample();
            accumulated_time += sample_time;
            UpdateSummer(accumulated_time);
            return sample_value;
        }
        void UpdateSummer(double time)
        {
            for (const auto& element : elements)
            {
                if (time >= element.delay_to_start && time < (element.delay_to_start + element.duration))
                {
                    if (!summer.HasSource(element.base_sound))
                    {
                        summer.AddSource(element.base_sound);
                    }
                }
                else
                {
                    summer.RemoveSource(element.base_sound);
                }   
            }
        }
        std::vector<sequence_element> elements;
        MutableSummer summer;
        double accumulated_time;
        double sample_time;
    };

    std::shared_ptr<ISampleSource> CreateSequence(std::vector<sequence_element> elements, double sample_rate)
    {
        return std::make_shared<SequenceSampleSource>(elements, sample_rate);
    }
}
