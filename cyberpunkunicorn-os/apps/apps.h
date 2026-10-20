// Every CyberUnicorn 5000 app, in one place. build.sh copies apps/ into the
// Bruce tree as src/cyber_apps/; patch 0006 includes this header from the main
// menu and the serial CLI. Adding an app: include its header here, then add it
// to CYBER_APP_MEMBERS / CYBER_APP_MENU_ITEMS / cyberAppsCreateCommands below.
#pragma once

#include "hat/HatMenu.h"
#include "lightshow/LightShowMenu.h"
#include "slideshow/SlideshowMenu.h"
#include "weather/WeatherMenu.h"
#include "weather/weather.h"

// Members of MainMenu (one instance per app).
#define CYBER_APP_MEMBERS                                                                         \
    HatMenu hatMenu;                                                                              \
    LightShowMenu lightShowMenu;                                                                  \
    SlideshowMenu slideshowMenu;                                                                  \
    WeatherMenu weatherMenu;

// Pointers listed first in the main menu, in this order.
#define CYBER_APP_MENU_ITEMS &lightShowMenu, &slideshowMenu, &weatherMenu, &hatMenu,

inline void cyberAppsCreateCommands(SimpleCLI *cli) {
    createHatCommand(cli);
    createSlideshowCommand(cli);
    createWeatherCommand(cli);
}
