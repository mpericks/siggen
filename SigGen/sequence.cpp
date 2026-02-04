#include <memory>
#include <vector>
#include <unordered_map>

#include "sequence.h"

namespace Neato
{
    class SampleSourceWithDuration : public ISampleSourceWithDuration
    {
    public:
        SampleSourceWithDuration(std::shared_ptr<ISampleSource> source_in, double duration_in, double sample_rate_in)
        : source(source_in)
        , duration(duration_in)
        , duration_in_samples(duration_in * sample_rate_in)
        , accumulated_samples(0)
        {
        }
        virtual double Sample() override
        {
            if (accumulated_samples <= duration_in_samples)
            {
                accumulated_samples++;
                return source->Sample();
            }
            return 0.0;
        }
        double Duration() const override
        {
            return duration;
        }
        void Reset() override
        {
            accumulated_samples = 0;
        }
    private:
        std::shared_ptr<ISampleSource> source;
        double duration;
        double duration_in_samples;
        double accumulated_samples;
    };

    struct SequenceMilestone
    {
        SequenceMilestone()
            :on_off(false)
        {
        }
        sequence_element element;
        bool on_off;
    };
    using milestone_map_t = std::unordered_multimap<uint64_t, SequenceMilestone>;

    class SequenceSampleSource : public ISampleSourceWithDuration
    {
    public:
        SequenceSampleSource(const std::vector<sequence_element>& elements_in, double sample_rate_in)
        : elements(elements_in)
        , summer()
        , accumulated_samples(0)
        , sample_time(1.0 / sample_rate_in)
        , duration(0.0)
        {
            CalculateDurationsMilestones();
            UpdateSummer(0);
        }
        virtual double Sample() override
        {
            double sample_value = summer.Sample();
            accumulated_samples++;
            UpdateSummer(accumulated_samples);
            return sample_value;
        }
        double Duration() const override
        {
            return duration;
        }
        void Reset() override
        {
            accumulated_samples = 0;
            summer.ClearSources();
            UpdateSummer(0);
        }
    private:
        void CalculateDurationsMilestones()
        {
            duration = 0.0;
            // loop through elements and find the sample count where notes turn on or off.
            // also calculate overall duration
            for (const auto& element : elements)
            {
                double element_end_time = element.delay_to_start + element.base_sound->Duration();
                if (element_end_time > duration)
                {
                    duration = element_end_time;
                }
                uint64_t start_sample = static_cast<uint64_t>(element.delay_to_start / sample_time);
                uint64_t end_sample = static_cast<uint64_t>((element.delay_to_start + element.base_sound->Duration()) / sample_time);
                SequenceMilestone start_milestone;
                start_milestone.element = element;
                start_milestone.on_off = true;
                milestones.insert({ start_sample, start_milestone });
                SequenceMilestone end_milestone;
                end_milestone.element = element;
                end_milestone.on_off = false;
                milestones.insert({end_sample, end_milestone});
            }
        }
        void UpdateSummer(uint64_t sample_count)
        {
            auto range = milestones.equal_range(static_cast<uint64_t>(sample_count));
            for (auto& it = range.first; it != range.second; ++it)
            {
                const SequenceMilestone& milestone = it->second;
                if (milestone.on_off)
                {
                    summer.AddSource(milestone.element.base_sound);
                }
                else
                {
                    summer.RemoveSource(milestone.element.base_sound);
                }
            }
        }
        std::vector<sequence_element> elements;
        MutableSummer summer;
        uint64_t accumulated_samples;
        const double sample_time;
        double duration;
        milestone_map_t milestones;
    };

    std::shared_ptr<ISampleSourceWithDuration> CreateSoundWithDuration(std::shared_ptr<ISampleSource> source, double duration, double sample_rate)
    {
        return std::make_shared<SampleSourceWithDuration>(source, duration, sample_rate);
    }

    std::shared_ptr<ISampleSourceWithDuration> CreateSequence(std::vector<sequence_element> elements, double sample_rate)
    {
        return std::make_shared<SequenceSampleSource>(elements, sample_rate);
    }
}
