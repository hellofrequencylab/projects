// Every CyberUnicorn 5000 app, in one place. build.sh copies apps/ into the
// Bruce tree as src/cyber_apps/; patch 0006 includes this header from the main
// menu and the serial CLI. Adding an app: include its header here, then add it
// to CYBER_APP_MEMBERS / CYBER_APP_MENU_ITEMS / cyberAppsCreateCommands below.
#pragma once

#include "frequency/FrequencyMenu.h"
#include "lightshow/LightShowMenu.h"
#include "sensors/SensorsMenu.h"
#include "light/LightMenu.h"
#include "sound/SoundMenu.h"
#include "slideshow/SlideshowMenu.h"
#include "weather/WeatherMenu.h"
#include "weather/weather.h" // declares createWeatherCommand
#include "hat/HatMenu.h"
#include "gpio/GpioMenu.h"
#include "remote/RemoteMenu.h"
#include "hardware/LibraryMenu.h"
#include "net/net.h" // no menu: console Wi-Fi commands + `netcheck`

// Members of MainMenu (one instance per app).
#define CYBER_APP_MEMBERS \
    FrequencyMenu frequencyMenu; \
    LightShowMenu lightShowMenu; \
    SensorsMenu sensorsMenu; \
    LightMenu lightMenu; \
    SoundMenu soundMenu; \
    SlideshowMenu slideshowMenu; \
    WeatherMenu weatherMenu; \
    HatMenu hatMenu; \
    GpioMenu gpioMenu; \
    RemoteMenu remoteMenu; \
    LibraryMenu libraryMenu;

// Pointers listed first in the main menu, in this order.
#define CYBER_APP_MENU_ITEMS &frequencyMenu, &lightShowMenu, &sensorsMenu, &lightMenu, &soundMenu, &slideshowMenu, &weatherMenu, &hatMenu, &gpioMenu, &remoteMenu,

// Listed just before Config (patch 0016). Apps whose hardware isn't found yet live
// in here instead of the main menu (hardware/hardware.h has the rules).
#define CYBER_APP_MENU_TAIL &libraryMenu,

inline void cyberAppsCreateCommands(SimpleCLI *cli) {
    createFrequencyCommand(cli);
    createSensorsCommand(cli);
    createLightCommand(cli);
    createSoundCommand(cli);
    createSlideshowCommand(cli);
    createWeatherCommand(cli);
    createHatCommand(cli);
    createPinsCommand(cli);
    createRemoteCommand(cli);
    createHardwareCommand(cli);
    createNetCommand(cli);
}
