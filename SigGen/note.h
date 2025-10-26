#pragma once
#include "base_waveforms.hpp"
#include "envelope.hpp"

namespace neato
{
	struct sequence_element
	{
		std::shared_ptr<ISampleSource> base_sound;
		double duration;
		double delay_to_start;
	};

	std::shared_ptr<ISampleSource> CreateSequence(std::vector<sequence_element> elements, double sample_rate);
}

