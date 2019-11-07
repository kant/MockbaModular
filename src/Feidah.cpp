// Simple voltage fader by Mockba the Borg

#include "plugin.hpp"

struct Feidah : Module {
	enum ParamIds {
		_KNOB_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		_VOLTAGE_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		_VOLTAGE_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		NUM_LIGHTS
	};

	Feidah() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(_KNOB_PARAM, 0.f, 1.f, 0.f, "");
	}

	void process(const ProcessArgs& args) override {
		float atten1 = params[_KNOB_PARAM].getValue();
		float out1 = inputs[_VOLTAGE_INPUT].getVoltage() * atten1;
		out1 = clamp(out1, -10.0f, 10.0f);
		outputs[_VOLTAGE_OUTPUT].setVoltage(out1);
	}
};

struct FeidahWidget : ModuleWidget {
	FeidahWidget(Feidah* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/Feidah.svg")));

		// Screws
		addChild(createWidget<_Screw>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<_Screw>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		// Knobs
		addParam(createParamCentered<_Knob>(mm2px(Vec(5.1, 57.0)), module, Feidah::_KNOB_PARAM));

		// Inputs
		addInput(createInputCentered<_Port>(mm2px(Vec(5.1, 90.0)), module, Feidah::_VOLTAGE_INPUT));

		// Outputs
		addOutput(createOutputCentered<_Port>(mm2px(Vec(5.1, 101.0)), module, Feidah::_VOLTAGE_OUTPUT));
	}
};

Model* modelFeidah = createModel<Feidah, FeidahWidget>("Feidah");