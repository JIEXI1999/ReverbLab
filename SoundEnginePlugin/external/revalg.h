#include "./delay.h"
#include "./mix-matrix.h"

#include "../../JuceModules/JuceHeader.h"

#include <cstdlib>

float randomInRange(float low, float high) {
	// There are better randoms than this, and you should use them instead 😛
	float unitRand = rand() / float(RAND_MAX);
	return low + unitRand * (high - low);
}

// This is a simple delay class which rounds to a whole number of samples.
using Delay = signalsmith::delay::Delay<float, signalsmith::delay::InterpolatorNearest>;

template<int channels = 8>
struct MultiChannelMixedFeedback {
	using Array = std::array<float, channels>;
	float delayMs = 150;
	float decayGain = 0.85;

	std::array<int, channels> delaySamples;
	std::array<Delay, channels> delays;

	void configure(float sampleRate) {
		float delaySamplesBase = delayMs * 0.001 * sampleRate;
		for (int c = 0; c < channels; ++c) {
			float r = c * 1.0 / channels;
			delaySamples[c] = std::pow(2, r) * delaySamplesBase;
			delays[c].resize(delaySamples[c] + 1);
			delays[c].reset();
		}
	}


	Array process(Array input) {
		Array delayed;
		for (int c = 0; c < channels; ++c) {
			delayed[c] = delays[c].read(delaySamples[c]);
		}

		// Mix using a Householder matrix
		Array mixed = delayed;
		Householder<float, channels>::inPlace(mixed.data());

		for (int c = 0; c < channels; ++c) {
			float sum = input[c] + mixed[c] * decayGain;
			delays[c].write(sum);
		}

		return delayed;
	}
};

template<int channels = 8>
struct DiffusionStep {
	using Array = std::array<float, channels>;
	float delayMsRange = 50;

	std::array<int, channels> delaySamples;
	std::array<Delay, channels> delays;
	std::array<bool, channels> flipPolarity;

	void configure(float sampleRate) {
		float delaySamplesRange = delayMsRange * 0.001 * sampleRate;
		for (int c = 0; c < channels; ++c) {
			float rangeLow = delaySamplesRange * c / channels;
			float rangeHigh = delaySamplesRange * (c + 1) / channels;
			delaySamples[c] = randomInRange(rangeLow, rangeHigh);
			delays[c].resize(delaySamples[c] + 1);
			delays[c].reset();
			flipPolarity[c] = rand() % 2;
		}
	}

	Array process(Array input) {
		// Delay
		Array delayed;
		for (int c = 0; c < channels; ++c) {
			delays[c].write(input[c]);
			delayed[c] = delays[c].read(delaySamples[c]);
		}

		// Mix with a Hadamard matrix
		Array mixed = delayed;
		Hadamard<float, channels>::inPlace(mixed.data());

		// Flip some polarities
		for (int c = 0; c < channels; ++c) {
			if (flipPolarity[c]) mixed[c] *= -1;
		}

		return mixed;
	}
};

template<int channels = 8, int stepCount = 4>
struct DiffuserEqualLengths {
	using Array = std::array<float, channels>;

	using Step = DiffusionStep<channels>;
	std::array<Step, stepCount> steps;

	DiffuserEqualLengths(float totalDiffusionMs) {
		for (auto& step : steps) {
			step.delayMsRange = totalDiffusionMs / stepCount;
		}
	}

	void configure(float sampleRate) {
		for (auto& step : steps) step.configure(sampleRate);
	}

	Array process(Array samples) {
		for (auto& step : steps) {
			samples = step.process(samples);
		}
		return samples;
	}
};

template<int channels = 8, int stepCount = 4>
struct DiffuserHalfLengths {
	using Array = std::array<float, channels>;

	using Step = DiffusionStep<channels>;
	std::array<Step, stepCount> steps;

	DiffuserHalfLengths(float diffusionMs) {
		stepDelayUpadate(diffusionMs);
	}

	void configure(float sampleRate) {
		for (auto& step : steps) step.configure(sampleRate);
	}

	void stepDelayUpadate(float diffusionMs) {
		for (auto& step : steps) {
			diffusionMs *= 0.5;
			step.delayMsRange = diffusionMs;
		}
	}

	Array process(Array samples) {
		for (auto& step : steps) {
			samples = step.process(samples);
		}
		return samples;
	}
};

template<int channels = 8, int diffusionSteps = 5>
struct BasicReverb {
	using Array = std::array<float, channels>;
	using Spec = juce::dsp::ProcessSpec;

	Spec reverbSpec;
	MultiChannelMixedFeedback<channels> feedback;
	DiffuserHalfLengths<channels, diffusionSteps> diffuser;
	juce::dsp::FirstOrderTPTFilter<float> filter;
	bool enable = false;

	float dry, wet,rt60, roomSizeMs, sampleRate;

	BasicReverb(float roomSizeMs, float rt60, float dry = 0, float wet = 1) 
		: diffuser(roomSizeMs), dry(dry), wet(wet),roomSizeMs(roomSizeMs) {
		feedback.delayMs = roomSizeMs;
		setRt60(rt60);
	}

	void configure(const Spec& spec) {
		reverbSpec = spec;
		feedback.configure(reverbSpec.sampleRate);
		diffuser.configure(reverbSpec.sampleRate);
		setupFilter();
		this->sampleRate = spec.sampleRate;
	}

	Array process(Array input) {
		Array diffuse = diffuser.process(input);
		Array longLasting = feedback.process(diffuse);
		Array output;
		for (int c = 0; c < channels; ++c) {
			if (enable){
				float filteredOutput = filter.processSample(c%2, longLasting[c]);
				output[c] = dry * input[c] + wet * filteredOutput;
			}
			else { output[c] = dry * input[c] + wet * longLasting[c]; }
		}
		return output;
	}

	void setupFilter() {
		filter.prepare(reverbSpec);
		filter.reset();
		filter.setType(juce::dsp::FirstOrderTPTFilterType::lowpass);
		filter.setCutoffFrequency(20000.f);
	}

	void setRt60(float newRt60) {
		rt60 = newRt60;
		updateDecayGain();
	}

	void setDamping(float damping) {
		enable = true;
		filter.setCutoffFrequency(damping);
		if (damping > 19500.f) { enable = false; }
	}

	void setGeometry(float geometry) {
	}

	void filterSnapToZero() {
		filter.snapToZero();
	}

private:
	void updateDecayGain() {
		float typicalLoopMs = roomSizeMs * 1.5;
		float loopsPerRt60 = rt60 / (typicalLoopMs * 0.001);
		float dbPerCycle = -60 / loopsPerRt60;
		feedback.decayGain = std::pow(10, dbPerCycle * 0.05);
	}

};