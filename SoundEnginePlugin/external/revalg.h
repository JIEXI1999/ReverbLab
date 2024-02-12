#include "./delay.h"
#include "./mix-matrix.h"

#include "../../JuceModules/JuceHeader.h"

#include <cstdlib>

float randomInRange(float low, float high) {
	float unitRand = rand() / float(RAND_MAX);
	return low + unitRand * (high - low);
}

// This is a simple delay class which rounds to a whole number of samples.
using Delay = signalsmith::delay::Delay<float, signalsmith::delay::InterpolatorNearest>;
using Spec = juce::dsp::ProcessSpec;
using IIRF = juce::dsp::IIR::Filter<float>;

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
			// Distribute delay times exponentially between delayMs and 2*delayMs
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

	// Decorrelate each channel's signal as much as possible for natural sounding
	Array process(Array input, IIRF& dampingFilter, bool enableDamping) {
		// Delay
		Array delayed;
		for (int c = 0; c < channels; ++c) {
			delays[c].write(input[c]);
			delayed[c] = delays[c].read(delaySamples[c]);
			if (enableDamping) 
				delayed[c] = dampingFilter.processSample(delayed[c]);
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

// Alternative to DiffuserHalfLengths. Not used in my plugin
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
			// This is adjustable before compiling if you want to change diffuse length pattern
			// Use 0.5 for half length every step
			diffusionMs *= 0.5; 
			// Use value defined above to initialize every single diffuser
			step.delayMsRange = diffusionMs;
		}
	}

	Array process(Array samples, IIRF& dampingFilter, bool enableDamping) {
		for (auto& step : steps) {
			samples = step.process(samples, dampingFilter, enableDamping);
		}
		return samples;
	}
};

template<int channels = 8, int diffusionSteps = 5>
struct BasicReverb {
	// Holding 8 channels' current sample
	using Array = std::array<float, channels>;

	Spec reverbSpec;
	MultiChannelMixedFeedback<channels> feedback;
	DiffuserHalfLengths<channels, diffusionSteps> diffuser;
	juce::dsp::IIR::Filter<float> highShelfFilter;
	bool enableDamping = false;

	float rt60, roomSizeMs, sampleRate;

	// Constructor
	BasicReverb(float roomSizeMs, float rt60) 
		: diffuser(roomSizeMs),roomSizeMs(roomSizeMs) {
		feedback.delayMs = roomSizeMs;
		setRt60(rt60);
	}

	void configure(const Spec& spec) {
		// Setup BasicReverb when Init()
		reverbSpec = spec;
		feedback.configure(reverbSpec.sampleRate);
		diffuser.configure(reverbSpec.sampleRate);
		setupFilter();
		this->sampleRate = spec.sampleRate;
	}

	Array process(Array input) {
		// Do diffuse and feedback processing successively for input signals
		Array diffuse = diffuser.process(input, highShelfFilter, enableDamping);
		Array longLasting = feedback.process(diffuse);
		Array output;
		for (int c = 0; c < channels; ++c) {
			output[c] = longLasting[c];
		}
		return output;
	}

	void setupFilter() {
		// Setup filter with default values when Init()
		highShelfFilter.reset();
		highShelfFilter.prepare(reverbSpec);
		auto coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf
		(reverbSpec.sampleRate, 15000.f, 0.5f, juce::Decibels::decibelsToGain(0.f));
		highShelfFilter.coefficients = coefficients;
	}

	void setRt60(float newRt60) {
		rt60 = newRt60;
		// recalculate decay gain if decay time changed
		updateDecayGain();
	}

	void setDamping(float cutoff, float attenuation) {
		enableDamping = true;
		// recalculate filter coefficients if HFCutoff or HFAttenuation updated by user
		auto coefficients = juce::dsp::IIR::Coefficients<float>::makeHighShelf
		(reverbSpec.sampleRate, cutoff, 0.5f, juce::Decibels::decibelsToGain(-attenuation));
		highShelfFilter.coefficients = coefficients;
		// reset filter's state everytime assigning new coefficients to avoid artifacts like clipping
		highShelfFilter.reset();
		// if damping is unneeded, turn off filter for optimization purpose
		if (cutoff > 14999.f) enableDamping = false;
	}

	void setGeometry(float geometry) {
	}

	void filterSnapToZero() {
		highShelfFilter.snapToZero();
	}

private:
	void updateDecayGain() {
		// How long does our signal take to go around the feedback loop?
		float typicalLoopMs = roomSizeMs * 1.5;
		// How many times will it do that during our RT60 period?
		float loopsPerRt60 = rt60 / (typicalLoopMs * 0.001);
		// This tells us how many dB to reduce per loop
		float dbPerCycle = -45 / loopsPerRt60;

		feedback.decayGain = std::pow(10, dbPerCycle * 0.05);
	}

};