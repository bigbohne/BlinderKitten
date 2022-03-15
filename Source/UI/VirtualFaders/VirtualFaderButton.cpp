/*
  ==============================================================================

	Object.cpp
	Created: 26 Sep 2020 10:02:32am
	Author:  bkupe

  ==============================================================================
*/

#include "JuceHeader.h"
#include "VirtualFaderButton.h"
#include "VirtualFaderColManager.h"
#include "VirtualFaderColGrid.h"
#include "../../Brain.h"

VirtualFaderButton::VirtualFaderButton(var params) :
	BaseItem(params.getProperty("name", "VirtualFaderButton")),
	objectType(params.getProperty("type", "VirtualFaderButton").toString()),
	objectData(params)
{
	saveAndLoadRecursiveData = true;
	nameCanBeChangedByUser = false;

	itemDataType = "VirtualFaderButton";

	targetType = addEnumParameter("Target type", "");
	targetType->addOption("disabled", "disabled");
	targetType->addOption("Same as column", "column");
	targetType->addOption("Cuelist", "cuelist");
	targetType->addOption("Effect", "effect");
	targetType->addOption("Carousel", "carousel");

	targetId = addIntParameter("Target ID", "", 0, 0);
	cuelistAction = addEnumParameter("Cuelist action", "");
	cuelistAction->addOption("Go", "go");
	cuelistAction->addOption("Off", "off");
	cuelistAction->addOption("Flash", "flash");
	cuelistAction->addOption("Load", "load");
	cuelistAction->addOption("Loadn and Go", "loadandgo");
	cuelistAction->addOption("Swop", "swop");
	cuelistAction->addOption("Go random", "gorandom");

	effectAction = addEnumParameter("Effect action", "");
	effectAction->addOption("Start", "start");
	effectAction->addOption("Stop", "stop");
	effectAction->addOption("Tap tempo", "taptempo");

	carouselAction = addEnumParameter("Carousel Action", "");
	carouselAction->addOption("Start", "start");
	carouselAction->addOption("Stop", "stop");
	carouselAction->addOption("Tap tempo", "taptempo");

	// id = addIntParameter("ID", "ID of this VirtualFaderButton", 1, 1);
	// userName = addStringParameter("Name", "Name of this VirtualFaderButton","New VirtualFaderButton");
	updateDisplay();
}

VirtualFaderButton::~VirtualFaderButton()
{
}

void VirtualFaderButton::updateName() {
}

void VirtualFaderButton::onContainerParameterChangedInternal(Parameter* c) {
	if (c == targetType) {
		updateDisplay();
	}
	VirtualFaderColGrid::getInstance()->fillCells();
}

void VirtualFaderButton::updateDisplay() {
	String targType = targetType->getValue();
	
	targetId->hideInEditor = targType == "column" || targType == "disabled" ;

	if (targType == "column" && parentContainer != nullptr) {
		targType = dynamic_cast<VirtualFaderCol*>(parentContainer->parentContainer.get())->targetType->getValue();
	}
	cuelistAction->hideInEditor = targType != "cuelist";
	effectAction->hideInEditor = targType != "effect";
	carouselAction->hideInEditor = targType != "carousel";

	queuedNotifier.addMessage(new ContainerAsyncEvent(ContainerAsyncEvent::ControllableContainerNeedsRebuild, this));
}

void VirtualFaderButton::pressed(String colTargetType, int colTargetId) {
	String targType = targetType->getValue();
	int targId = targetId->getValue();
	
	if (targType == "column") {
		targType = colTargetType;
		targId = colTargetId;
	}

	if (targId == 0) {return;}

	if (targType == "cuelist") {
		Cuelist* targ = Brain::getInstance()->getCuelistById(targId);
		if (targ != nullptr) {
			String action = cuelistAction->getValue();
			if (action == "go") { targ->go(); }
			if (action == "off") { targ->off(); }
			if (action == "flash") { targ->flash(true, false, false); }
			if (action == "load") { targ->showLoad(); }
			if (action == "loadandgo") { targ->showLoadAndGo(); }
			if (action == "swop") { targ->flash(true, false, true); }
			if (action == "gorandom") { targ->goRandom(); }
		}
	}
	else if (targType == "effect") {
		Effect* targ = Brain::getInstance()->getEffectById(targId);
		if (targ != nullptr) {
			String action = effectAction->getValue();
			if (action == "start") { targ->start(); }
			if (action == "stop") { targ->stop(); }
			if (action == "taptempo") { targ->tapTempo(); }
		}
	}
	else if (targType == "carousel") {
		Carousel* targ = Brain::getInstance()->getCarouselById(targId);
		if (targ != nullptr) {
			String action = carouselAction->getValue();
			if (action == "start") { targ->start(); }
			if (action == "stop") { targ->stop(); }
			if (action == "taptempo") { targ->tapTempo(); }
		}
	}
}

void VirtualFaderButton::released(String colTargetType, int colTargetId) {
	String targType = targetType->getValue();
	int targId = targetId->getValue();

	if (targType == "column") {
		targType = colTargetType;
		targId = colTargetId;
	}

	if (targId == 0) { return; }

	if (targType == "cuelist") {
		Cuelist* targ = Brain::getInstance()->getCuelistById(targId);
		if (targ != nullptr) {
			String action = cuelistAction->getValue();
			if (action == "flash") { targ->flash(false, false, false); }
			if (action == "swop") { targ->flash(false, false, true); }
		}
	}
	else if (targType == "effect") {
		Effect* targ = Brain::getInstance()->getEffectById(targId);
		if (targ != nullptr) {
			String action = effectAction->getValue();
			// if (action == "start") { targ->start(); }
		}
	}
	else if (targType == "carousel") {
		Carousel* targ = Brain::getInstance()->getCarouselById(targId);
		if (targ != nullptr) {
			String action = carouselAction->getValue();
			// if (action == "start") { targ->start(); }
		}
	}

}

String VirtualFaderButton::getBtnText(String columnType) {
	String text = "";
	String targType = targetType->getValue();
	String action = "";
	if (targType == "column" && columnType != "") {
		targType = columnType;
		if (targType == "cuelist") {
			action = cuelistAction->getValue();
		}
		else if (targType == "effect") {
			action = effectAction->getValue();
		}
		else if (targType == "carousel") {
			action = carouselAction->getValue();
		}

		return action;
	}
	else {
		int targId = targetId->getValue();
		if (targId == 0) { return ""; }

		if (targType == "cuelist") {
			Cuelist* targ = Brain::getInstance()->getCuelistById(targId);
			action = cuelistAction->getValue();
			if (targ != nullptr) {
				text = targ->userName->getValue();
			}
		}
		else if (targType == "effect") {
			Effect* targ = Brain::getInstance()->getEffectById(targId);
			action = effectAction->getValue();
			if (targ != nullptr) {
				text = targ->userName->getValue();
			}
		}
		else if (targType == "carousel") {
			Carousel* targ = Brain::getInstance()->getCarouselById(targId);
			action = carouselAction->getValue();
			if (targ != nullptr) {
				text = targ->userName->getValue();
			}
		}

		if (text != "") {
			text = action + "\n" + text;
		}
		return text;
	}
}
