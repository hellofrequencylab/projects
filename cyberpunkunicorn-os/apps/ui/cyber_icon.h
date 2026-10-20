// CyberUnicorn 5000 pixel-art app icons (design/icons) and the skinned menus
// that show them. Patch 0010 calls the menu functions from Bruce's main menu,
// submenu and option loop, so every menu item with an icon in
// cyber_icons.h gets it without its app being edited.
#pragma once

#include <Arduino.h>
#include <vector>

struct Option;

// 25 fps animation tick, as in the boot scene.
inline int cyberIconTick() { return (int)(millis() / 40); }

// Draws the icon for menu item `menu` centred on (cx, cy) at `cell` px per
// icon cell (3 = 96x72). With `animate`, the menu idle loop keeps it moving
// until the screen is cleared. False if there is no icon for that name.
bool cyberIcon(const char *menu, int cx, int cy, int cell, bool animate = false);

// The friendly label for a menu name ("Frequency" -> "Frequency Check-in"),
// or nullptr when it has no icon.
const char *cyberIconLabel(const char *menu);

// Main-menu page: big icon between two arrows, rainbow label, page dots.
// `item` is the MenuItemInterface pointer the option was built with. False
// when the item has no icon (Bruce then draws it the old way).
bool cyberMainMenuItem(void *item, const String &menu);

// loopOptions(MENU_TYPE_SUBMENU) as a list: icon header when `title` is a
// menu name, neon rows, selected row filled, scroll counter.
void cyberOptionList(int index, std::vector<Option> &options, const char *title);

// Menu loops call this every pass: steps the animated icon every 40 ms.
void cyberIconIdle();

// The screen was cleared: forget the animated icon and the list layout.
void cyberIconForget();
