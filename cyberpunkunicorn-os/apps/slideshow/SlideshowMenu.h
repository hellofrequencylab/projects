#pragma once

#include <MenuItemInterface.h>
#include <SimpleCLI.h>

// "Slideshow": cycles JPG/PNG/BMP/GIF pictures from /slides (SD card, else
// LittleFS). Stills hold for the slide time, GIFs play through once. Ported
// from the CircuitPython Unicorn OS launcher (apps/slideshow.py).
class SlideshowMenu : public MenuItemInterface {
public:
    SlideshowMenu() : MenuItemInterface("Slideshow") {}

    void optionsMenu(void) override;
    void drawIcon(float scale) override;
    bool hasTheme() override { return false; }
    const String &themePath() override { return _noTheme; }

private:
    String _noTheme = "";
    void run();
};

// Serial: `slides` lists what the slideshow would play and its settings.
void createSlideshowCommand(SimpleCLI *cli);
