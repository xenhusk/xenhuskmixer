#include "MainComponent.h"

#include <BinaryData.h>

#include <juce_gui_extra/juce_gui_extra.h>

namespace
{

juce::Image loadAppIcon()
{
    return juce::ImageFileFormat::loadFrom(BinaryData::icon_png, BinaryData::icon_pngSize);
}

class MainWindow : public juce::DocumentWindow
{
public:
    explicit MainWindow(juce::String name)
        : DocumentWindow(name, juce::Colour(0xff1D1D1F), DocumentWindow::allButtons)
    {
        icon = loadAppIcon();
        setUsingNativeTitleBar(true);
        setContentOwned(new xenhusk::MainComponent(), true);
        setResizable(true, true);
        setResizeLimits(1440, 860, 4096, 2160);
        centreWithSize(1880, 1000);
        setVisible(true);
        applyIcon();
    }

    void closeButtonPressed() override
    {
        hideMixer();
    }

    void showMixer()
    {
        setMinimised(false);
        setVisible(true);
        toFront(true);
        applyIcon();
    }

    void showAtDefaultPosition()
    {
        setMinimised(false);
        centreWithSize(1880, 1000);
        setVisible(true);
        toFront(true);
        applyIcon();
    }

    void hideMixer()
    {
        setVisible(false);
    }

    void restartAudioEngine()
    {
        if (auto* mixer = dynamic_cast<xenhusk::MainComponent*>(getContentComponent()))
            mixer->restartAudioEngine();
    }

private:
    void applyIcon()
    {
        if (! icon.isValid())
            return;

        setIcon(icon);
        if (auto* peer = getPeer())
            peer->setIcon(icon);
    }

    juce::Image icon;
};

class MixerTray : public juce::SystemTrayIconComponent
{
public:
    MixerTray(MainWindow& owner, juce::Image icon)
        : window(owner)
    {
        auto trayIcon = icon.isValid() ? icon.rescaled(32, 32, juce::Graphics::highResamplingQuality) : icon;
        setIconImage(trayIcon, trayIcon);
        setIconTooltip("Xenhusk Mixer");
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        if (event.mods.isPopupMenu())
            showMenu();
        else
            window.showMixer();
    }

    void mouseDoubleClick(const juce::MouseEvent& event) override
    {
        if (! event.mods.isPopupMenu())
            window.showMixer();
    }

private:
    void showMenu()
    {
        juce::PopupMenu menu;
        menu.addItem(1, "Restart Audio Engine");
        menu.addItem(2, "Show Xenhusk Mixer");
        menu.addItem(3, "Show Xenhusk Mixer (default position)");
        menu.addItem(4, "Hide Xenhusk Mixer");
        menu.addSeparator();
        menu.addItem(5, "Shut Down Xenhusk Mixer");
        menu.addItem(6, "Exit Menu");

        menu.showMenuAsync(juce::PopupMenu::Options(),
                           [this](int result)
                           {
                               if (result == 1)
                                   window.restartAudioEngine();
                               else if (result == 2)
                                   window.showMixer();
                               else if (result == 3)
                                   window.showAtDefaultPosition();
                               else if (result == 4)
                                   window.hideMixer();
                               else if (result == 5)
                                   juce::JUCEApplication::getInstance()->systemRequestedQuit();
                           });
    }

    MainWindow& window;
};

} // namespace

class XenhuskMixerApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Xenhusk Mixer"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
        tray = std::make_unique<MixerTray>(*mainWindow, loadAppIcon());
    }

    void shutdown() override
    {
        tray = nullptr;
        mainWindow = nullptr;
    }

private:
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<MixerTray> tray;
};

START_JUCE_APPLICATION(XenhuskMixerApplication)
