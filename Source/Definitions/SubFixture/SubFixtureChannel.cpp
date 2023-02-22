/*
  ==============================================================================

	Object.cpp
	Created: 26 Sep 2020 10:02:32am
	Author:  bkupe

  ==============================================================================
*/

#include "JuceHeader.h"
#include "SubFixtureChannel.h"
#include "../ChannelFamily/ChannelFamilyManager.h"
#include "../FixtureType/FixtureTypeChannel.h"
#include "../ChannelFamily/ChannelType/ChannelType.h"
#include "Brain.h"
#include "../Fixture/FixturePatch.h"
#include "../Interface/InterfaceIncludes.h"
#include "../Cuelist/Cuelist.h"
#include "../Programmer/Programmer.h"
#include "../Effect/Effect.h"
#include "../ChannelValue.h"
#include "UI/InputPanel.h"

SubFixtureChannel::SubFixtureChannel():
	virtualChildren()
{
	cs.enter();
	cuelistStack.clear();
	programmerStack.clear();
	effectStack.clear();
	carouselStack.clear();
	cuelistFlashStack.clear();
	virtualChildren.clear();
	cs.exit();
}

SubFixtureChannel::~SubFixtureChannel()
{
	//isDeleted = true;
	cs.enter();
	Brain::getInstance()->usingCollections.enter();
	Brain::getInstance()->grandMasterChannels.removeAllInstancesOf(this);
	Brain::getInstance()->swoppableChannels.removeAllInstancesOf(this);
	for (int i = Brain::getInstance()->subFixtureChannelPoolWaiting.size() - 1; i >= 0; i--) {
		if (Brain::getInstance()->subFixtureChannelPoolWaiting[i] == this) {
			Brain::getInstance()->subFixtureChannelPoolWaiting[i] = nullptr;
		}
	}
	for (int i = Brain::getInstance()->subFixtureChannelPoolUpdating.size() - 1; i >= 0; i--) {
		if (Brain::getInstance()->subFixtureChannelPoolUpdating[i] == this) {
			Brain::getInstance()->subFixtureChannelPoolUpdating[i] = nullptr;
		}
	}
	Brain::getInstance()->usingCollections.exit();

	cs.exit();
}

void SubFixtureChannel::writeValue(float v) {
	v= jmin((float)1, v);
	v= jmax((float)0, v);

	if (virtualChildren.size() > 0) {
		value = v;
		for (int i = 0; i < virtualChildren.size(); i++) {
			Brain::getInstance()->pleaseUpdate(virtualChildren[i]);
		}
		return;
	}

	if (virtualMaster != nullptr) {
		v *= virtualMaster->value;
	}

	if (parentFixture != nullptr && parentFixtureTypeChannel != nullptr && parentParamDefinition != nullptr) {

		int deltaAdress = parentFixtureTypeChannel->dmxDelta->getValue();
		deltaAdress--;
		String chanRes = parentFixtureTypeChannel->resolution->getValue();
		Array<FixturePatch*> patchs = parentFixture->patchs.getItemsWithType<FixturePatch>();
		for (int i = 0; i < patchs.size(); i++) {
			if (patchs[i]->enabled->boolValue()) {
				value = v;

				DMXInterface* out = dynamic_cast<DMXInterface*>(patchs.getReference(i)->targetInterface->targetContainer.get());
				if (out != nullptr) {
					FixturePatch* patch = patchs.getReference(i);
					int address = patch->address->getValue();

					for (int iCorr = 0; iCorr < patch->corrections.items.size(); iCorr++) {
						FixturePatchCorrection* c = patch->corrections.items[iCorr];
						if (c->isOn && c->enabled->boolValue() && (int)c->subFixtureId->getValue() == subFixtureId && dynamic_cast<ChannelType*>(c->channelType->targetContainer.get()) == channelType) {
							if (c->invertChannel->getValue()) {
								value = 1 - value;
							}
							value = value + (float)c->offsetValue->getValue();
							value = jmin((float)1, value);
							value = jmax((float)0, value);
							value = c->curve.getValueAtPosition(value);
						}
					}


					if (address > 0) {
						address += (deltaAdress);
						if (chanRes == "8bits") {
							int val = floor(256.0 * value);
							val = val > 255 ? 255 : val;
							out->sendDMXValue(address, val);
						}
						else if (chanRes == "16bits") {
							int val = floor(65535.0 * value);
							val = value > 65535 ? 65535 : val;
							int valueA = val / 256;
							int valueB = val % 256;
							out->sendDMXValue(address, valueA);
							out->sendDMXValue(address + 1, valueB);
						}
					}
				}
			}
		}

	}

}


void SubFixtureChannel::updateVal(double now) {
	float newValue = defaultValue;

	cs.enter();
	Array<int> layers;

	for (int i = 0; i < cuelistStack.size(); i++) {
		layers.addIfNotAlreadyThere(cuelistStack.getReference(i)->layerId->getValue());
	}
	for (int i = 0; i < programmerStack.size(); i++) {
		layers.addIfNotAlreadyThere(programmerStack.getReference(i)->layerId->getValue());
	}
	for (int i = 0; i < mapperStack.size(); i++) {
		layers.addIfNotAlreadyThere(mapperStack.getReference(i)->layerId->getValue());
	}
	for (int i = 0; i < carouselStack.size(); i++) {
		layers.addIfNotAlreadyThere(carouselStack.getReference(i)->layerId->getValue());
	}
	for (int i = 0; i < effectStack.size(); i++) {
		layers.addIfNotAlreadyThere(effectStack.getReference(i)->layerId->getValue());
	}

	layers.sort();
	
	bool checkSwop = Brain::getInstance()->isSwopping && swopKillable;

	for (int l = 0; l< layers.size(); l++)
		{
		int currentLayer = layers.getReference(l);

		int overWritten = -1;
		for (int i = 0; i < cuelistStack.size(); i++)
		{
			Cuelist* c = cuelistStack.getReference(i);
			if ((int)c->layerId->getValue() == currentLayer) {
				if (!checkSwop || c->isSwopping) {
					newValue = c->applyToChannel(this, newValue, now);
					ChannelValue* cv = c->activeValues.getReference(this);
					if (cv != nullptr && cv->isEnded) {
						overWritten = i - 1;
					}
				}
			}
		}

		postCuelistValue = newValue;

		for (int i = 0; i <= overWritten; i++) {
			if ((int)cuelistStack.getReference(i)->layerId->getValue() == currentLayer) {
					ChannelValue* cv = cuelistStack.getReference(i)->activeValues.getReference(this);
				if (cv != nullptr && !cv->isOverWritten) {
					cv->isOverWritten = true;
					Brain::getInstance()->pleaseUpdate(cuelistStack.getReference(i));
				}
			}
		}
		for (int i = 0; i < programmerStack.size(); i++) {
			if ((int)programmerStack.getReference(i)->layerId->getValue() == currentLayer) {
				newValue = programmerStack.getReference(i)->applyToChannel(this, newValue, now);
			}
		}

		for (int i = 0; i < mapperStack.size(); i++) {
			if ((int)mapperStack.getReference(i)->layerId->getValue() == currentLayer) {
				newValue = mapperStack.getReference(i)->applyToChannel(this, newValue, now);
			}
		}

		for (int i = 0; i < carouselStack.size(); i++) {
			if ((int)carouselStack.getReference(i)->layerId->getValue() == currentLayer) {
				newValue = carouselStack.getReference(i)->applyToChannel(this, newValue, now);
			}
		}

		for (int i = 0; i < effectStack.size(); i++) {
			if ((int)effectStack.getReference(i)->layerId->getValue() == currentLayer) {
				newValue = effectStack.getReference(i)->applyToChannel(this, newValue, now);
			}
		}
	
	}

	for (int i = 0; i < cuelistFlashStack.size(); i++)
	{
		// newValue = cuelistFlashStack.getReference(i)->applyToChannel(this, newValue, now, true);
	}

	/*

	if (swopKillable) {
		if (Brain::getInstance()->isSwopping) {
			newValue = defaultValue;
			for (int i = 0; i < cuelistFlashStack.size(); i++)
			{
				if (cuelistFlashStack.getReference(i)->isSwopping) {
					newValue = cuelistFlashStack.getReference(i)->applyToChannel(this, newValue, now, true);
				}
			}
		}
	}
	*/
	if (reactToGrandMaster) {
		double gm = InputPanel::getInstance()->grandMaster.getValue();
		newValue *= gm;
	}

	cs.exit();
	writeValue(newValue);
}

void SubFixtureChannel::cuelistOnTopOfStack(Cuelist* c) {
	cuelistOutOfStack(c);
	cuelistStack.add(c);
}

void SubFixtureChannel::cuelistOutOfStack(Cuelist* c) {
	while (cuelistStack.indexOf(c) >= 0) {
		cuelistStack.removeAllInstancesOf(c);
	}
}

void SubFixtureChannel::cuelistOnTopOfFlashStack(Cuelist* c) {
	cuelistOutOfFlashStack(c);
	cuelistFlashStack.add(c);
}

void SubFixtureChannel::cuelistOutOfFlashStack(Cuelist* c) {
	while (cuelistFlashStack.indexOf(c) >= 0) {
		cuelistFlashStack.removeAllInstancesOf(c);
	}
}

void SubFixtureChannel::programmerOnTopOfStack(Programmer* p) {
	programmerOutOfStack(p);
	programmerStack.add(p);
}

void SubFixtureChannel::programmerOutOfStack(Programmer* p) {
	while (programmerStack.indexOf(p) >= 0) {
		programmerStack.removeAllInstancesOf(p);
	}
}

void SubFixtureChannel::effectOnTopOfStack(Effect* f) {
	effectOutOfStack(f);
	effectStack.add(f);
}

void SubFixtureChannel::effectOutOfStack(Effect* f) {
	while (effectStack.indexOf(f) >= 0) {
		effectStack.removeAllInstancesOf(f);
	}
}

void SubFixtureChannel::carouselOnTopOfStack(Carousel* f) {
	carouselOutOfStack(f);
	carouselStack.add(f);
}

void SubFixtureChannel::carouselOutOfStack(Carousel* f) {
	while (carouselStack.indexOf(f) >= 0) {
		carouselStack.removeAllInstancesOf(f);
	}
}

void SubFixtureChannel::mapperOnTopOfStack(Mapper* f) {
	mapperOutOfStack(f);
	mapperStack.add(f);
}

void SubFixtureChannel::mapperOutOfStack(Mapper* f) {
	while (mapperStack.indexOf(f) >= 0) {
		mapperStack.removeAllInstancesOf(f);
	}
}

