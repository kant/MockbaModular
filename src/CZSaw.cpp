// Casio CZ style Saw oscillator by Mockba the Borg

#include "plugin.hpp"
#include "MockbaModular.hpp"

template <int OVERSAMPLE, int QUALITY, typename T>
struct _Saw {
	T freq;
	T shape;
	T phase = 0.f;
	T outValue = 0.f;

	dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> oscMinBlep;

	void setPitch(T pitchV) {
		freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitchV + 30) / 1073741824;
		for (int i = 0; i < 4; i++)
			freq[i] += i / DETUNE;
	}

	void setShape(T shapeV) {
		shape = simd::clamp(shapeV, 0.1f, 9.9f) * 0.1f;
	}

	void process(float delta) {
		// Calculate phase
		T deltaPhase = simd::clamp(freq * delta, 1e-6f, 0.35f);
		phase += deltaPhase;
		phase -= simd::floor(phase);

		outValue = oscStep(phase, shape);
		outValue += oscMinBlep.process();
	}

	T oscStep(T phase, T shape) {
		// Calculate the wave step
		T l = 0.5f - (shape * 0.5f);
		T a = phase * ((0.5f - l) / l);
		T b = (-phase + 1.f) * ((0.5f - l) / (1.f - l));
		T m = phase + simd::fmin(a, b);
		T v = simd::cos(m * M_2PI);
		return v;
	}

	T out() {
		return outValue;
	}
};

struct CZSaw : Module {
	enum ParamIds {
		_LFO_PARAM,
		_FREQ_PARAM,
		_FINE_PARAM,
		_SHAPE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		_MODF_INPUT,
		_MODS_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		_WAVE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	_Saw<16, 16, float_4> osc[4];

	CZSaw() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(_LFO_PARAM, 0, 1, 0, "OFF ON");
		configParam(_FREQ_PARAM, -54.f, 54.f, 0.f, "Frequency", " Hz", dsp::FREQ_SEMITONE, dsp::FREQ_C4);
		configParam(_FINE_PARAM, -1.f, 1.f, 0.f, "Fine frequency / LFO Offset");
		configParam(_SHAPE_PARAM, 0.f, 10.f, 0.f, "Shape");
	}

	void onAdd() override;

	void onReset() override;

	void process(const ProcessArgs& args) override;
};

void CZSaw::onAdd() {
}

void CZSaw::onReset() {
	onAdd();
}

void CZSaw::process(const ProcessArgs& args) {
	// Get the frequency parameters
	float freqParam = params[_FREQ_PARAM].getValue() / 12.f;
	// LFO mode
	if (params[_LFO_PARAM].getValue())
		freqParam = (freqParam * 2) - 5;
	freqParam += dsp::quadraticBipolar(params[_FINE_PARAM].getValue()) * 3.f / 12.f;
	// Get the shape parameter
	float shapeParam = params[_SHAPE_PARAM].getValue();
	// Iterate over each channel
	int channels = max(inputs[_MODF_INPUT].getChannels(), 1);
	for (int c = 0; c < channels; c += 4) {
		// Get the oscillator
		auto* oscillator = &osc[c / 4];
		float_4 pitch = freqParam;
		// Set the pitch
		pitch += inputs[_MODF_INPUT].getVoltageSimd<float_4>(c);
		oscillator->setPitch(pitch);
		// Set the shape
		float_4 shape = shapeParam;
		shape += inputs[_MODS_INPUT].getVoltageSimd<float_4>(c);
		oscillator->setShape(shape);
		// Process and output
		oscillator->process(args.sampleTime);
		float_4 off = params[_LFO_PARAM].getValue() * params[_FINE_PARAM].getValue() * 5.f;
		outputs[_WAVE_OUTPUT].setVoltageSimd(5.f * oscillator->out() + off, c);
	}
	outputs[_WAVE_OUTPUT].setChannels(channels);
}

struct CZSawWidget : ModuleWidget {
	CZSawWidget(CZSaw* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, BGCOLOR)));
		SvgWidget* panel = createWidget<SvgWidget>(Vec(0, 0));
		panel->setSvg(APP->window->loadSvg(asset::plugin(pluginInstance, "res/CZSaw.svg")));
		addChild(panel);

		// Screws
		addChild(createWidget<_Screw>(Vec(0, 0)));
		addChild(createWidget<_Screw>(Vec(box.size.x - RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Knobs
		addParam(createParamCentered<_Hsw>(mm2px(Vec(5.1, 46.0)), module, CZSaw::_LFO_PARAM));
		addParam(createParamCentered<_Knob>(mm2px(Vec(5.1, 57.0)), module, CZSaw::_FREQ_PARAM));
		addParam(createParamCentered<_Knob>(mm2px(Vec(5.1, 68.0)), module, CZSaw::_FINE_PARAM));
		addParam(createParamCentered<_Knob>(mm2px(Vec(5.1, 90.0)), module, CZSaw::_SHAPE_PARAM));

		// Inputs
		addInput(createInputCentered<_Port>(mm2px(Vec(5.1, 79.0)), module, CZSaw::_MODF_INPUT));
		addInput(createInputCentered<_Port>(mm2px(Vec(5.1, 101.0)), module, CZSaw::_MODS_INPUT));

		// Outputs
		addOutput(createOutputCentered<_Port>(mm2px(Vec(5.1, 112.0)), module, CZSaw::_WAVE_OUTPUT));
	}
};

Model* modelCZSaw = createModel<CZSaw, CZSawWidget>("CZSaw");