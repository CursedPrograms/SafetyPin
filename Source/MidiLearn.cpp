#include "MidiLearn.h"

namespace
{
const juce::Identifier stateType { "MidiLearn" };
}

MidiLearn::MidiLearn (juce::AudioProcessorValueTreeState& p, const juce::StringArray& parameterIds)
    : params (p), ids (parameterIds)
{
    jassert (ids.size() <= maxParameters);
    for (auto& c : ccToParameter)
        c = -1;
    for (auto& v : pending)
        v = -1.0f;
    startTimerHz (60);
}

MidiLearn::~MidiLearn()
{
    stopTimer();
}

void MidiLearn::process (const juce::MidiBuffer& midi)
{
    for (const auto metadata : midi)
    {
        const auto message = metadata.getMessage();
        if (! message.isController())
            continue;
        const auto cc = message.getControllerNumber();

        if (const auto learn = learnParameter.load(); learn >= 0)
        {
            for (auto& c : ccToParameter)            // one CC per knob
                if (c == learn)
                    c = -1;
            ccToParameter[(size_t) cc] = learn;
            learnParameter = -1;
            mappingChanged = true;
        }

        if (const auto parameter = ccToParameter[(size_t) cc].load(); parameter >= 0)
            pending[(size_t) parameter] = (float) message.getControllerValue() / 127.0f;
    }
}

void MidiLearn::startLearning (int parameter)
{
    learnParameter = parameter;
}

void MidiLearn::forget (int parameter)
{
    for (auto& c : ccToParameter)
        if (c == parameter)
            c = -1;
    if (learnParameter == parameter)
        learnParameter = -1;
    mappingChanged = true;
}

int MidiLearn::ccFor (int parameter) const
{
    for (size_t cc = 0; cc < ccToParameter.size(); ++cc)
        if (ccToParameter[cc] == parameter)
            return (int) cc;
    return -1;
}

void MidiLearn::timerCallback()
{
    for (int i = 0; i < ids.size(); ++i)
    {
        const auto value = pending[(size_t) i].exchange (-1.0f);
        if (value < 0.0f)
            continue;
        if (auto* parameter = params.getParameter (ids[i]))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (value);
            parameter->endChangeGesture();
        }
    }
    if (mappingChanged.exchange (false))
        saveToState();
}

void MidiLearn::saveToState()
{
    auto tree = params.state.getOrCreateChildWithName (stateType, nullptr);
    tree.removeAllProperties (nullptr);
    for (size_t cc = 0; cc < ccToParameter.size(); ++cc)
        if (const auto parameter = ccToParameter[cc].load(); parameter >= 0)
            tree.setProperty ("cc" + juce::String ((int) cc), ids[parameter], nullptr);
}

void MidiLearn::loadFromState()
{
    for (auto& c : ccToParameter)
        c = -1;
    const auto tree = params.state.getChildWithName (stateType);
    for (int i = 0; i < tree.getNumProperties(); ++i)
    {
        const auto name = tree.getPropertyName (i).toString();
        const auto cc = name.fromFirstOccurrenceOf ("cc", false, false).getIntValue();
        const auto parameter = ids.indexOf (tree[tree.getPropertyName (i)].toString());
        if (name.startsWith ("cc") && cc >= 0 && cc < 128 && parameter >= 0)
            ccToParameter[(size_t) cc] = parameter;
    }
}
