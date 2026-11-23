//------------------------------------------------------------------------
// Copyright(c) 2026 My Plug-in Company.
//------------------------------------------------------------------------

#include "processor.h"
#include "cids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

using namespace Steinberg;

namespace MyCompanyName {
//------------------------------------------------------------------------
// vst002Processor
//------------------------------------------------------------------------
vst002Processor::vst002Processor ()
{
	//--- set the wanted controller for our processor
	setControllerClass (kvst002ControllerUID);
}

//------------------------------------------------------------------------
vst002Processor::~vst002Processor ()
{}

//------------------------------------------------------------------------
tresult PLUGIN_API vst002Processor::initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated
	
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	// if everything Ok, continue
	if (result != kResultOk)
	{
		return result;
	}

	//--- create Audio IO ------
	addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

	/* If you don't need an event bus, you can remove the next line */
	addEventInput (STR16 ("Event In"), 1);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API vst002Processor::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!
	
	//---do not forget to call parent ------
	return AudioEffect::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API vst002Processor::setActive (TBool state)
{
	//--- called when the Plug-in is enable/disable (On/Off) -----
	return AudioEffect::setActive (state);
}

//------------------------------------------------------------------------
tresult PLUGIN_API vst002Processor::process (Vst::ProcessData& data)
{
	//--- First : Read inputs parameter changes-----------

	if (data.inputParameterChanges)
	{
		int32 numParamsChanged = data.inputParameterChanges->getParameterCount ();
		for (int32 index = 0; index < numParamsChanged; index++)
		{
			if (auto* paramQueue = data.inputParameterChanges->getParameterData (index))
			{
				Vst::ParamValue value;
				int32 sampleOffset;
				int32 numPoints = paramQueue->getPointCount ();
				switch (paramQueue->getParameterId ())
				{
				case GainParams::kGainId:
					if (paramQueue->getPoint (numPoints - 1, sampleOffset, value) == kResultTrue)
					{
						mGain = value;
					}
					break;
				}
			}
		}
	}

	//---Second: Read input events------------
	// get the list of all event changes
	Vst::IEventList* eventList = data.inputEvents;
	if (eventList)
	{
		int32 numEvent = eventList->getEventCount();
		for (int32 i = 0; i < numEvent; i++)
		{
			Vst::Event event;
			if (eventList->getEvent(i, event) == kResultOk)
			{
				switch (event.type)
				{
					case Vst::Event::kNoteOnEvent:
						mGainReduction = event.noteOn.velocity;
						break;
					case Vst::Event::kNoteOffEvent:
						mGainReduction = 0.f;
						break;
				}
			}
		}
	}
	
	//--- Here you have to implement your processing

	if (data.numSamples > 0)
	{
		//--- ------------------------------------------
		// here as example a default implementation where we apply a simple gain:
		// the gain is attenuation only (normalized parameter value in [0, 1])
		// if less input than outputs then clear outputs
		//--- ------------------------------------------
		
		// mGain is a normalized parameter value (ParamValue = double), while the
		// buffers are Sample32 (float): convert once instead of per sample
		const Vst::Sample32 gain = static_cast<Vst::Sample32> (mGain - mGainReduction);

		int32 minBus = std::min (data.numInputs, data.numOutputs);
		for (int32 i = 0; i < minBus; i++)
		{
			int32 minChan = std::min (data.inputs[i].numChannels, data.outputs[i].numChannels);
			for (int32 c = 0; c < minChan; c++)
			{
				const Vst::Sample32* in  = data.inputs[i].channelBuffers32[c];
				Vst::Sample32*       out = data.outputs[i].channelBuffers32[c];

				// in-place is fine: every sample is read before the same index is written
				for (int32 s = 0; s < data.numSamples; s++)
					out[s] = in[s] * gain;
			}

			// the output is silent where the input is silent,
			// and everywhere when the gain mutes it completely
			data.outputs[i].silenceFlags = (gain == 0.0f)
				? (((uint64)1 << minChan) - 1)
				: data.inputs[i].silenceFlags;
				
			// clear the remaining output buffers
			for (int32 c = minChan; c < data.outputs[i].numChannels; c++)
			{
				// clear output buffers
				memset (data.outputs[i].channelBuffers32[c], 0,
						data.numSamples * sizeof (Vst::Sample32));

				// inform the host that this channel is silent
				data.outputs[i].silenceFlags |= ((uint64)1 << c);
			}
		}
		// clear the remaining output buffers
		for (int32 i = minBus; i < data.numOutputs; i++)
		{
			// clear output buffers
			for (int32 c = 0; c < data.outputs[i].numChannels; c++)
			{
				memset (data.outputs[i].channelBuffers32[c], 0,
						data.numSamples * sizeof (Vst::Sample32));
			}
			// inform the host that this bus is silent
			data.outputs[i].silenceFlags = ((uint64)1 << data.outputs[i].numChannels) - 1;
		}
	}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API vst002Processor::setupProcessing (Vst::ProcessSetup& newSetup)
{
	//--- called before any processing ----
	return AudioEffect::setupProcessing (newSetup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API vst002Processor::canProcessSampleSize (int32 symbolicSampleSize)
{
	// by default kSample32 is supported
	if (symbolicSampleSize == Vst::kSample32)
		return kResultTrue;

	// disable the following comment if your processing support kSample64
	/* if (symbolicSampleSize == Vst::kSample64)
		return kResultTrue; */

	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API vst002Processor::setState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	// called when we load a preset, the model has to be reloaded
	IBStreamer streamer (state, kLittleEndian);
	float savedParam1 = 0.f;
	if (streamer.readFloat(savedParam1) == false)
		return kResultFalse;
	mGain = savedParam1;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API vst002Processor::getState (IBStream* state)
{
	// here we need to save the model
	float toSaveParam1 = mGain;
	IBStreamer streamer (state, kLittleEndian);
	streamer.writeFloat (toSaveParam1);
	return kResultOk;
}

//------------------------------------------------------------------------
} // namespace MyCompanyName
