/*
  ==============================================================================

    MIDIInterface.cpp
    Created: 12 Oct 2020 11:07:40am
    Author:  bkupe

  ==============================================================================
*/

#include "Definitions/Interface/InterfaceIncludes.h"

MIDIInterface::MIDIInterface() :
    Interface(getTypeString()),
    inputDevice(nullptr),
    outputDevice(nullptr),
    dataContainer("Start sysex content")
{
    deviceParam = new MIDIDeviceParameter("Device");

    addParameter(deviceParam);
    numBytes = addIntParameter("Start sysex", "Sysex message to send at startup, F0 and F7 are automatically added at the begin and the end, so don't fill them :) .", 0, 0);

    addChildControllableContainer(&dataContainer);
    updateBytesParams();

    addChildControllableContainer(&mappingManager);
    addChildControllableContainer(&feedbackManager);
}

MIDIInterface::~MIDIInterface()
{
    if (inputDevice != nullptr) inputDevice->removeMIDIInputListener(this);
    if (outputDevice != nullptr) outputDevice->close();
}

void MIDIInterface::updateDevices()
{
    if (isClearing) return;

    if (deviceParam->inputDevice != inputDevice)
    {
        if (inputDevice != nullptr) inputDevice->removeMIDIInputListener(this);
        inputDevice = deviceParam->inputDevice;

        if (inputDevice != nullptr)
        {
            inputDevice->addMIDIInputListener(this);
            NLOG(niceName, "Now listening to MIDI Device : " << inputDevice->name);
        }
    }

    if (deviceParam->outputDevice != outputDevice) {
        MIDIOutputDevice* newOutput = deviceParam->outputDevice;
        if (outputDevice != nullptr)
        {
            outputDevice->close();
        }

        outputDevice = newOutput;

        if (outputDevice != nullptr)
        {
            outputDevice->open();
            NLOG(niceName, "Now writing to MIDI Device : " << outputDevice->name);
            sendStartupBytes();
        }
    }
}

void MIDIInterface::onContainerParameterChangedInternal(Parameter* p)
{
    if (p == deviceParam)
    {
        updateDevices();
    }
    if (p == numBytes) 
    {
        updateBytesParams();
    }
}

void MIDIInterface::sendStartupBytes()
{
    if (numBytes->intValue() == 0) return;
    if (outputDevice == nullptr) return; 

    Array<uint8> data;
    for (auto& c : dataContainer.controllables) data.add(((IntParameter*)c)->intValue());
    outputDevice->sendSysEx(data);
}

void MIDIInterface::noteOnReceived(const int &channel, const int &pitch, const int &velocity)
{
    if (!enabled->boolValue()) {return;}
    if (logIncomingData->boolValue()) NLOG(niceName, "Note On received, channel : " << channel << ", pitch : " << pitch << ", velocity : " << velocity);
    mappingManager.handleNote(channel, pitch, velocity, niceName);
}

void MIDIInterface::noteOffReceived(const int &channel, const int &pitch, const int &velocity)
{
    if (!enabled->boolValue()) { return; }
    if (logIncomingData->boolValue()) NLOG(niceName, "Note Off received, channel : " << channel << ", pitch : " << pitch << ", velocity : " << velocity);
    mappingManager.handleNote(channel, pitch, 0, niceName);
}

void MIDIInterface::controlChangeReceived(const int &channel, const int &number, const int &value)
{
    if (!enabled->boolValue()) { return; }
    if (logIncomingData->boolValue()) NLOG(niceName, "Control Change received, channel : " << channel << ", number : " << number << ", value : " << value);
    mappingManager.handleCC(channel, number, value, niceName);
}

void MIDIInterface::pitchWheelReceived(const int& channel, const int& value)
{
    if (!enabled->boolValue()) { return; }
    if (logIncomingData->boolValue()) NLOG(niceName, "Pitch wheel received, channel : " << channel << ", value : " << value);
    mappingManager.handlePitchWheel(channel, value, niceName);
}

void MIDIInterface::feedback(String address, var value, String origin = "")
{
    bool logOutput = logOutgoingData->boolValue();
    for (int i = 0; i < feedbackManager.items.size(); i++) {
        MIDIFeedback* f = feedbackManager.items[i];
        f->processFeedback(address, value, origin, logOutput);
    }
}


void MIDIInterface::updateBytesParams()
{
    while (dataContainer.controllables.size() > numBytes->intValue())
    {
        dataContainer.controllables[dataContainer.controllables.size() - 1]->remove();
    }

    while (dataContainer.controllables.size() < numBytes->intValue())
    {
        String index = String(dataContainer.controllables.size());
        IntParameter* p = new IntParameter("#" + index, "Data for the byte #" + index, 0, 0, 255);
        p->hexMode = true;
        dataContainer.addParameter(p); // after hexMode
    }

}
