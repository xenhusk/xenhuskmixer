#include "SettingsDocument.h"

namespace xenhusk
{
namespace
{

const char* busAttribute[Mixer::busCount] = { "busa", "busa2", "busa3", "busa4", "busa5", "busb", "busb2", "busb3" };

void writeBand(juce::XmlElement& parent, const char* tag, int index, int channel, int band, Mixer::EqBand value)
{
    auto* element = parent.createNewChildElement(tag);
    element->setAttribute("index", index);
    element->setAttribute("channel", channel);
    element->setAttribute("band", band);
    element->setAttribute("enabled", value.enabled ? 1 : 0);
    element->setAttribute("type", value.type);
    element->setAttribute("frequency", value.frequency);
    element->setAttribute("gain", value.gainDb);
    element->setAttribute("q", value.q);
}

float unitOrDecibels(float value)
{
    if (std::abs(value) <= 1.0f)
        return std::clamp(value, 0.0f, 1.0f);

    return std::clamp(dbToLinear(value), 0.0f, 1.0f);
}

void applyOwn(Mixer& mixer, const juce::XmlElement& root)
{
    mixer.resetToDefault();

    for (auto* element = root.getFirstChildElement(); element != nullptr; element = element->getNextElement())
    {
        if (element->hasTagName("Strip"))
        {
            const int strip = element->getIntAttribute("index", -1);
            if (strip < 0 || strip >= Mixer::stripCount)
                continue;

            mixer.setStripName(strip, element->getStringAttribute("name"));
            mixer.setFaderDb(strip, (float) element->getDoubleAttribute("fader", 0.0));
            mixer.setStripMuted(strip, element->getIntAttribute("mute") != 0);
            mixer.setStripSolo(strip, element->getIntAttribute("solo") != 0);
            mixer.setStripMono(strip, element->getIntAttribute("mono") != 0);
            mixer.setEqEnabled(strip, element->getIntAttribute("eq") != 0);
            mixer.setPostFx(strip, element->getIntAttribute("post", 1) != 0);
            mixer.setColor(strip, (float) element->getDoubleAttribute("color"));
            mixer.setVoice(strip, (float) element->getDoubleAttribute("voice"));
            mixer.setEcho(strip, (float) element->getDoubleAttribute("echo"));
            mixer.setComp(strip, (float) element->getDoubleAttribute("comp"));
            mixer.setGate(strip, (float) element->getDoubleAttribute("gate"));
            mixer.setDenoise(strip, (float) element->getDoubleAttribute("denoise"));
            mixer.setBass(strip, (float) element->getDoubleAttribute("bass"));
            mixer.setTreble(strip, (float) element->getDoubleAttribute("treble"));
            mixer.setReverbSend(strip, (float) element->getDoubleAttribute("reverb"));
            mixer.setDelaySend(strip, (float) element->getDoubleAttribute("delay"));
            mixer.setPan(strip, (float) element->getDoubleAttribute("pan"));
            mixer.setModulation(strip, (float) element->getDoubleAttribute("modulation"));
            mixer.setInputPair(strip, element->getIntAttribute("input", strip));

            for (int bus = 0; bus < Mixer::busCount; ++bus)
                mixer.setAssigned(strip, bus, element->getIntAttribute(busAttribute[bus]) != 0);

            Mixer::CompSettings comp;
            comp.detailed = element->getIntAttribute("compDetailed") != 0;
            comp.inputDb = (float) element->getDoubleAttribute("compIn");
            comp.thresholdDb = (float) element->getDoubleAttribute("compThreshold", -18.0);
            comp.ratio = (float) element->getDoubleAttribute("compRatio", 1.0);
            comp.attackMs = (float) element->getDoubleAttribute("compAttack", 10.0);
            comp.releaseMs = (float) element->getDoubleAttribute("compRelease", 50.0);
            comp.knee = (float) element->getDoubleAttribute("compKnee", 6.0);
            comp.outputDb = (float) element->getDoubleAttribute("compOut");
            comp.autoMakeup = element->getIntAttribute("compAuto") != 0;
            mixer.setCompSettings(strip, comp);

            Mixer::GateSettings gate;
            gate.detailed = element->getIntAttribute("gateDetailed") != 0;
            gate.thresholdDb = (float) element->getDoubleAttribute("gateThreshold", -40.0);
            gate.rangeDb = (float) element->getDoubleAttribute("gateRange", -60.0);
            gate.attackMs = (float) element->getDoubleAttribute("gateAttack", 10.0);
            gate.holdMs = (float) element->getDoubleAttribute("gateHold", 200.0);
            gate.releaseMs = (float) element->getDoubleAttribute("gateRelease", 200.0);
            mixer.setGateSettings(strip, gate);
        }
        else if (element->hasTagName("EqBand"))
        {
            Mixer::EqBand band;
            band.enabled = element->getIntAttribute("enabled", 1) != 0;
            band.type = element->getIntAttribute("type");
            band.frequency = (float) element->getDoubleAttribute("frequency", 1000.0);
            band.gainDb = (float) element->getDoubleAttribute("gain");
            band.q = (float) element->getDoubleAttribute("q", 1.0);
            const int index = element->getIntAttribute("index");
            const int bandIndex = element->getIntAttribute("band");
            if (element->getStringAttribute("target") == "bus")
                mixer.setBusEqBand(index, bandIndex, band);
            else
            {
                const int channel = element->getIntAttribute("channel");
                mixer.setEqBand(index, channel, bandIndex, band);
            }
        }
        else if (element->hasTagName("Bus"))
        {
            const int bus = element->getIntAttribute("index", -1);
            if (bus < 0 || bus >= Mixer::busCount)
                continue;

            mixer.setBusFaderDb(bus, (float) element->getDoubleAttribute("fader"));
            mixer.setBusMuted(bus, element->getIntAttribute("mute") != 0);
            mixer.setBusMono(bus, element->getIntAttribute("mono") != 0);
            mixer.setBusEqEnabled(bus, element->getIntAttribute("eq") != 0);
            mixer.setBusBass(bus, (float) element->getDoubleAttribute("bass"));
            mixer.setBusTreble(bus, (float) element->getDoubleAttribute("treble"));
            mixer.setBusLimiter(bus, element->getIntAttribute("limiter") != 0);
        }
        else if (element->hasTagName("Effects"))
        {
            mixer.setReverbReturn((float) element->getDoubleAttribute("reverb"));
            mixer.setDelayReturn((float) element->getDoubleAttribute("delay"));
            mixer.setReverbDecay((float) element->getDoubleAttribute("decay", 1.0));
            mixer.setReverbPredelay((float) element->getDoubleAttribute("predelay"));
            mixer.setReverbEarly((float) element->getDoubleAttribute("early"));
            mixer.setDelayTime((float) element->getDoubleAttribute("delayTime", 0.18));
            mixer.setDelayFeedback((float) element->getDoubleAttribute("feedback", 0.35));

            for (int tap = 0; tap < Mixer::delayTaps; ++tap)
            {
                const auto prefix = "tap" + juce::String(tap);
                mixer.setDelayTap(tap, element->getIntAttribute(prefix + "on", tap == 0 ? 1 : 0) != 0,
                                  (float) element->getDoubleAttribute(prefix + "time", 0.18 * (tap + 1)),
                                  (float) element->getDoubleAttribute(prefix + "gain", tap == 0 ? 1.0 : 0.0));
            }
        }
    }
}

void applyVoiceMeeter(Mixer& mixer, const juce::XmlElement& root)
{
    mixer.resetToDefault();
    float loudestReturn = 0.0f;

    const auto visit = [&](const auto& self, const juce::XmlElement& element) -> void
    {
        const auto tag = element.getTagName();

        if (tag.startsWith("LabelVirtualStrip"))
        {
            const auto name = element.getAllSubText().trim();
            if (name.isNotEmpty())
                mixer.setStripName(4 + tag.getTrailingIntValue(), name);
        }
        else if (tag.startsWith("LabelStrip"))
        {
            const auto name = element.getAllSubText().trim();
            if (name.isNotEmpty())
                mixer.setStripName(tag.getTrailingIntValue() - 1, name);
        }
        else if (tag == "StripComp" || tag == "StripGate" || tag == "Strip")
        {
            const int strip = element.getIntAttribute("index", 0) - 1;
            if (strip >= 0 && strip < Mixer::stripCount)
            {
                if (element.hasAttribute("cell"))
                {
                    const int channel = std::clamp(element.getIntAttribute("channel", 1) - 1, 0, 1);
                    const int band = element.getIntAttribute("cell", 1) - 1;
                    Mixer::EqBand value = mixer.getEqBand(strip, channel, band);
                    value.enabled = element.getIntAttribute("EQon", 1) != 0;
                    value.type = element.getIntAttribute("EQtype");
                    value.frequency = (float) element.getDoubleAttribute("freq", value.frequency);
                    value.gainDb = (float) element.getDoubleAttribute("dblevel");
                    value.q = (float) element.getDoubleAttribute("Q", value.q);
                    mixer.setEqBand(strip, channel, band, value);
                }
                else if (tag == "StripComp")
                {
                    Mixer::CompSettings comp = mixer.getCompSettings(strip);
                    comp.detailed = true;
                    comp.inputDb = (float) element.getDoubleAttribute("gainin");
                    comp.thresholdDb = (float) element.getDoubleAttribute("threshold", comp.thresholdDb);
                    comp.ratio = (float) element.getDoubleAttribute("comprate", comp.ratio);
                    comp.attackMs = (float) element.getDoubleAttribute("attack", comp.attackMs);
                    comp.releaseMs = (float) element.getDoubleAttribute("release", comp.releaseMs);
                    comp.knee = (float) element.getDoubleAttribute("knee", comp.knee) * 12.0f;
                    comp.outputDb = (float) element.getDoubleAttribute("gainout");
                    comp.autoMakeup = element.getIntAttribute("automakeup") != 0;
                    mixer.setCompSettings(strip, comp);
                }
                else if (tag == "StripGate")
                {
                    Mixer::GateSettings gate = mixer.getGateSettings(strip);
                    gate.detailed = true;
                    gate.thresholdDb = (float) element.getDoubleAttribute("thresin", gate.thresholdDb);
                    gate.rangeDb = (float) element.getDoubleAttribute("damping", gate.rangeDb);
                    gate.attackMs = std::max(0.1f, (float) element.getDoubleAttribute("attack", gate.attackMs));
                    gate.holdMs = (float) element.getDoubleAttribute("hold", gate.holdMs);
                    gate.releaseMs = std::max(5.0f, (float) element.getDoubleAttribute("release", gate.releaseMs));
                    mixer.setGateSettings(strip, gate);
                }
                else
                {
                    if (element.hasAttribute("mute")) mixer.setStripMuted(strip, element.getIntAttribute("mute") != 0);
                    if (element.hasAttribute("solo")) mixer.setStripSolo(strip, element.getIntAttribute("solo") != 0);
                    if (element.hasAttribute("mono")) mixer.setStripMono(strip, element.getIntAttribute("mono") != 0);
                    if (element.hasAttribute("EQon")) mixer.setEqEnabled(strip, element.getIntAttribute("EQon") != 0);
                    if (element.hasAttribute("dblevel")) mixer.setFaderDb(strip, (float) element.getDoubleAttribute("dblevel"));
                    if (element.hasAttribute("prg_c")) mixer.setComp(strip, element.getIntAttribute("prg_c") != 0 ? 1.0f : 0.0f);
                    if (element.hasAttribute("prg_g")) mixer.setGate(strip, element.getIntAttribute("prg_g") != 0 ? 1.0f : 0.0f);
                    if (element.hasAttribute("prg_d")) mixer.setDenoise(strip, element.getIntAttribute("prg_d") != 0 ? 1.0f : 0.0f);
                    if (element.hasAttribute("sendR")) mixer.setReverbSend(strip, unitOrDecibels((float) element.getDoubleAttribute("sendR")));
                    if (element.hasAttribute("sendD")) mixer.setDelaySend(strip, unitOrDecibels((float) element.getDoubleAttribute("sendD")));
                    if (element.hasAttribute("postR") || element.hasAttribute("postD"))
                        mixer.setPostFx(strip, element.getIntAttribute("postR") != 0 || element.getIntAttribute("postD") != 0);
                    if (element.hasAttribute("ColorPanelx")) mixer.setColor(strip, (float) element.getDoubleAttribute("ColorPanelx"));
                    if (element.hasAttribute("ColorPanely")) mixer.setVoice(strip, std::abs((float) element.getDoubleAttribute("ColorPanely")));
                    if (element.hasAttribute("Panel3Dx")) mixer.setPan(strip, (float) element.getDoubleAttribute("Panel3Dx"));
                    if (element.hasAttribute("denoiser_g") && element.getIntAttribute("prg_d") != 0)
                        mixer.setDenoise(strip, std::clamp((float) element.getDoubleAttribute("denoiser_g") / 10.0f, 0.0f, 1.0f));
                    if (element.hasAttribute("busa") || element.hasAttribute("busb"))
                        for (int bus = 0; bus < Mixer::busCount; ++bus)
                            mixer.setAssigned(strip, bus, element.getIntAttribute(busAttribute[bus]) != 0);
                }
            }
        }
        else if (tag == "Bus" && ! element.hasAttribute("cell"))
        {
            const int bus = element.getIntAttribute("index", 0) - 1;
            if (bus >= 0 && bus < Mixer::busCount)
            {
                if (element.hasAttribute("mute")) mixer.setBusMuted(bus, element.getIntAttribute("mute") != 0);
                if (element.hasAttribute("mono")) mixer.setBusMono(bus, element.getIntAttribute("mono") != 0);
                if (element.hasAttribute("EQon")) mixer.setBusEqEnabled(bus, element.getIntAttribute("EQon") != 0);
                if (element.hasAttribute("dblevel")) mixer.setBusFaderDb(bus, (float) element.getDoubleAttribute("dblevel"));
                if (element.hasAttribute("retR"))
                    loudestReturn = std::max(loudestReturn, unitOrDecibels((float) element.getDoubleAttribute("retR")));
                if (element.hasAttribute("retD"))
                    mixer.setDelayReturn(std::max(mixer.getDelayReturn(), unitOrDecibels((float) element.getDoubleAttribute("retD"))));
            }
        }
        else if (tag == "Bus" && element.hasAttribute("cell"))
        {
            const int bus = element.getIntAttribute("index", 0) - 1;
            const int band = element.getIntAttribute("cell", 1) - 1;
            if (bus >= 0 && bus < Mixer::busCount && element.getIntAttribute("channel", 1) == 1)
            {
                Mixer::EqBand value = mixer.getBusEqBand(bus, band);
                value.enabled = element.getIntAttribute("EQon", 1) != 0;
                value.type = element.getIntAttribute("EQtype");
                value.frequency = (float) element.getDoubleAttribute("freq", value.frequency);
                value.gainDb = (float) element.getDoubleAttribute("dblevel");
                value.q = (float) element.getDoubleAttribute("Q", value.q);
                mixer.setBusEqBand(bus, band, value);
            }
        }
        else if (tag == "ReverbParam")
        {
            mixer.setReverbPredelay((float) element.getDoubleAttribute("predelay"));
            mixer.setReverbDecay((float) element.getDoubleAttribute("decay", 1.0));
            mixer.setReverbEarly(std::clamp((float) element.getDoubleAttribute("EarlyRefAmount"), 0.0f, 1.0f));
        }
        else if (tag == "ReverbGeneral")
        {
            if (element.hasAttribute("wet"))
                mixer.setReverbReturn((float) element.getDoubleAttribute("wet"));
        }

        for (auto* child = element.getFirstChildElement(); child != nullptr; child = child->getNextElement())
            self(self, *child);
    };

    visit(visit, root);
    if (loudestReturn > 0.0f)
        mixer.setReverbReturn(std::max(mixer.getReverbReturn(), loudestReturn));
}

} // namespace

std::unique_ptr<juce::XmlElement> settingsToXml(const Mixer& mixer)
{
    auto root = std::make_unique<juce::XmlElement>("XenhuskMixerSettings");
    root->setAttribute("version", 1);

    for (int strip = 0; strip < Mixer::stripCount; ++strip)
    {
        auto* element = root->createNewChildElement("Strip");
        element->setAttribute("index", strip);
        element->setAttribute("name", mixer.getStripName(strip));
        element->setAttribute("fader", mixer.getFaderDb(strip));
        element->setAttribute("mute", mixer.isStripMuted(strip) ? 1 : 0);
        element->setAttribute("solo", mixer.isStripSolo(strip) ? 1 : 0);
        element->setAttribute("mono", mixer.isStripMono(strip) ? 1 : 0);
        element->setAttribute("eq", mixer.isEqEnabled(strip) ? 1 : 0);
        element->setAttribute("post", mixer.isPostFx(strip) ? 1 : 0);
        element->setAttribute("color", mixer.getColor(strip));
        element->setAttribute("voice", mixer.getVoice(strip));
        element->setAttribute("echo", mixer.getEcho(strip));
        element->setAttribute("comp", mixer.getComp(strip));
        element->setAttribute("gate", mixer.getGate(strip));
        element->setAttribute("denoise", mixer.getDenoise(strip));
        element->setAttribute("bass", mixer.getBass(strip));
        element->setAttribute("treble", mixer.getTreble(strip));
        element->setAttribute("reverb", mixer.getReverbSend(strip));
        element->setAttribute("delay", mixer.getDelaySend(strip));
        element->setAttribute("pan", mixer.getPan(strip));
        element->setAttribute("modulation", mixer.getModulation(strip));
        element->setAttribute("input", mixer.getInputPair(strip));

        for (int bus = 0; bus < Mixer::busCount; ++bus)
            element->setAttribute(busAttribute[bus], mixer.isAssigned(strip, bus) ? 1 : 0);

        const auto comp = mixer.getCompSettings(strip);
        element->setAttribute("compDetailed", comp.detailed ? 1 : 0);
        element->setAttribute("compIn", comp.inputDb);
        element->setAttribute("compThreshold", comp.thresholdDb);
        element->setAttribute("compRatio", comp.ratio);
        element->setAttribute("compAttack", comp.attackMs);
        element->setAttribute("compRelease", comp.releaseMs);
        element->setAttribute("compKnee", comp.knee);
        element->setAttribute("compOut", comp.outputDb);
        element->setAttribute("compAuto", comp.autoMakeup ? 1 : 0);

        const auto gate = mixer.getGateSettings(strip);
        element->setAttribute("gateDetailed", gate.detailed ? 1 : 0);
        element->setAttribute("gateThreshold", gate.thresholdDb);
        element->setAttribute("gateRange", gate.rangeDb);
        element->setAttribute("gateAttack", gate.attackMs);
        element->setAttribute("gateHold", gate.holdMs);
        element->setAttribute("gateRelease", gate.releaseMs);

        for (int channel = 0; channel < 2; ++channel)
            for (int band = 0; band < Mixer::eqBands; ++band)
                writeBand(*root, "EqBand", strip, channel, band, mixer.getEqBand(strip, channel, band));
    }

    for (int bus = 0; bus < Mixer::busCount; ++bus)
    {
        auto* element = root->createNewChildElement("Bus");
        element->setAttribute("index", bus);
        element->setAttribute("fader", mixer.getBusFaderDb(bus));
        element->setAttribute("mute", mixer.isBusMuted(bus) ? 1 : 0);
        element->setAttribute("mono", mixer.isBusMono(bus) ? 1 : 0);
        element->setAttribute("eq", mixer.isBusEqEnabled(bus) ? 1 : 0);
        element->setAttribute("bass", mixer.getBusBass(bus));
        element->setAttribute("treble", mixer.getBusTreble(bus));
        element->setAttribute("limiter", mixer.isBusLimiter(bus) ? 1 : 0);

        for (int band = 0; band < Mixer::eqBands; ++band)
        {
            auto* eq = root->createNewChildElement("EqBand");
            const auto value = mixer.getBusEqBand(bus, band);
            eq->setAttribute("target", "bus");
            eq->setAttribute("index", bus);
            eq->setAttribute("band", band);
            eq->setAttribute("enabled", value.enabled ? 1 : 0);
            eq->setAttribute("type", value.type);
            eq->setAttribute("frequency", value.frequency);
            eq->setAttribute("gain", value.gainDb);
            eq->setAttribute("q", value.q);
        }
    }

    auto* effects = root->createNewChildElement("Effects");
    effects->setAttribute("reverb", mixer.getReverbReturn());
    effects->setAttribute("delay", mixer.getDelayReturn());
    effects->setAttribute("decay", mixer.getReverbDecay());
    effects->setAttribute("predelay", mixer.getReverbPredelay());
    effects->setAttribute("early", mixer.getReverbEarly());
    effects->setAttribute("delayTime", mixer.getDelayTime());
    effects->setAttribute("feedback", mixer.getDelayFeedback());

    for (int tap = 0; tap < Mixer::delayTaps; ++tap)
    {
        const auto prefix = "tap" + juce::String(tap);
        effects->setAttribute(prefix + "on", mixer.isDelayTapOn(tap) ? 1 : 0);
        effects->setAttribute(prefix + "time", mixer.getDelayTapTime(tap));
        effects->setAttribute(prefix + "gain", mixer.getDelayTapGain(tap));
    }

    return root;
}

bool applySettingsXml(Mixer& mixer, const juce::XmlElement& root)
{
    if (root.hasTagName("XenhuskMixerSettings"))
    {
        applyOwn(mixer, root);
        return true;
    }

    if (root.hasTagName("VBAudioVoicemeeterSettings"))
    {
        applyVoiceMeeter(mixer, root);
        return true;
    }

    return false;
}

juce::String repairSettingsText(juce::String text)
{
    text = text.replace("<VoiceMeeterParameters>\r\n<VoiceMeeterParameters>", "<VoiceMeeterParameters>");
    text = text.replace("<VoiceMeeterParameters>\n<VoiceMeeterParameters>", "<VoiceMeeterParameters>");
    text = text.replace("<VoiceMeeterParameters><VoiceMeeterParameters>", "<VoiceMeeterParameters>");
    return text;
}

bool loadSettingsFile(Mixer& mixer, const juce::File& file)
{
    const auto document = juce::XmlDocument::parse(repairSettingsText(file.loadFileAsString()));
    return document != nullptr && applySettingsXml(mixer, *document);
}

bool saveSettingsFile(const Mixer& mixer, const juce::File& file)
{
    const auto xml = settingsToXml(mixer);
    return xml != nullptr && xml->writeTo(file);
}

} // namespace xenhusk
