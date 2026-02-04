#pragma once
#include "base_waveforms.hpp"
#include "envelope.hpp"

namespace Neato
{
	struct ISampleSourceWithDuration : public ISampleSource
	{
		virtual double Duration() const = 0;
		virtual void Reset() = 0;
		virtual ~ISampleSourceWithDuration() = 0 {};
    };

	struct sequence_element
	{
		sequence_element() : delay_to_start(0.0) {}
		std::shared_ptr<ISampleSourceWithDuration> base_sound;
		double delay_to_start;
	};

    std::shared_ptr<ISampleSourceWithDuration> CreateSoundWithDuration(std::shared_ptr<ISampleSource> source, double duration, double sample_rate);
	std::shared_ptr<ISampleSourceWithDuration> CreateSequence(std::vector<sequence_element> elements, double sample_rate);
}

