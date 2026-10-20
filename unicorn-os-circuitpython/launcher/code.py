# Unicorn OS: the app menu. Tap BOOT to move down, double-tap to move up, hold to open.
# Inside an app, double-tap BOOT to come back.
import gc
import sys
import time
import traceback
import microcontroller
import ui

# 8x8 pixel icons ('.' is see-through)
ICONS = {
    "rainbow": (("..RRRR..", ".ROOOOR.", "ROYYYYOR", "OYGGGGYO", "YGB..BGY", "GB....BG",
                 "B......B", "........"),
                {"R": ui.BAD, "O": 0xFFC48A, "Y": 0xFFF08A, "G": ui.GOOD, "B": ui.ACCENT}),
    "heart": ((".PP..PP.", "PPPPPPPP", "PWPPPPPP", "PPPPPPPP", ".PPPPPP.", "..PPPP..",
               "...PP...", "........"), {"P": ui.PINK, "W": ui.FG}),
    "mountain": (("........", "...W....", "..WLL...", ".LLLLL..", ".LLLLLL.", "LLLLLLLL",
                  "GGGGGGGG", "........"), {"W": ui.FG, "L": ui.PURPLE, "G": ui.GOOD}),
    "sparkle": (("...Y....", "...Y....", "..YYY...", "YYYYYYY.", "..YYY...", "...Y....",
                 "...Y....", "........"), {"Y": ui.ACCENT}),
    "crystal": (("..LLLL..", ".LWLLLL.", "LLLLLLLL", ".LLLLLL.", "..LLLL..", "...LL...",
                 "........", "........"), {"L": ui.PURPLE, "W": ui.FG}),
    "key": (("..YYY...", ".Y...Y..", ".Y...Y..", "..YYY...", "...Y....", "...YY...",
             "...Y....", "...YY..."), {"Y": ui.GOLD}),
    "flower": (("..P.P...", ".PPPPP..", "..PYP...", ".PPPPP..", "..P.P...", "...G....",
                "..GG....", "...G...."), {"P": ui.PINK, "Y": ui.GOLD, "G": ui.GOOD}),
}

# (module in apps/, name, tagline, colour, icon)
APPS = [
    ("lightshow", "Light Show", "rainbow magic", ui.PINK, "rainbow"),
    ("system", "System", "vitals + settings", ui.BAD, "heart"),
    ("altimeter", "Altimeter", "height + logs", ui.PURPLE, "mountain"),
    ("wifi_radar", "Wi-Fi Radar", "networks near", ui.ACCENT, "sparkle"),
    ("ble_scanner", "BT Scanner", "devices near", ui.PURPLE, "crystal"),
    ("slideshow", "Slideshow", "pics + gifs", ui.PINK, "flower"),
    ("wifi_setup", "Wi-Fi Setup", "new network", ui.GOOD, "key"),
]

ROW_H = 16
TOP = 20

btn = ui.Button()
px = ui.Pixel()
ui.apply_settings(px)
if ui.wifi_networks() and ui.settings["wifi"]:
    g = ui.screen()
    g.append(ui.sprite(ui.UNICORN, ui.UNICORN_COLORS, ui.W // 2 - 24, 30, 4))
    g.append(ui.lbl("joining wi-fi...", ui.W // 2, 112, ui.PINK, anchor=(0.5, 0.5)))
    px.color(ui.ACCENT)
    ui.connect_wifi()
ui.state["px"] = px
try:
    import webui
    webui.start()
except (ImportError, OSError, RuntimeError) as e:
    print("no control panel:", e)
twinkles = []
HINTS = ("tap: next", "2x tap: up", "hold: open")
hint = []


def draw_menu(sel):
    g = ui.screen()
    ui.rainbow_bar(g, 0, 16, ui.W, 2)
    g.append(ui.sprite(ui.UNICORN, ui.UNICORN_COLORS, 3, 2))
    g.append(ui.lbl("unicorn os", 19, 8, ui.PINK))
    hint.clear()
    hint.append(ui.lbl(HINTS[0], ui.W - 30, 8, ui.DIM, anchor=(1.0, 0.5)))
    g.append(hint[0])
    twinkles.clear()
    for n, (x, y) in enumerate(((ui.W - 22, 6), (ui.W - 13, 10), (ui.W - 5, 5))):
        s = ui.star(x, y, 3, ui.RAINBOW[n * 2])
        g.append(s)
        twinkles.append(s)
    for i, (_, name, tag, color, icon) in enumerate(APPS):
        y = TOP + i * ROW_H
        if i == sel:
            # rounded pill behind the selected row
            g.append(ui.rect(9, y, ui.W - 18, ROW_H - 2, ui.PANEL))
            g.append(ui.circle(9, y + (ROW_H - 2) // 2, (ROW_H - 2) // 2, ui.PANEL))
            g.append(ui.circle(ui.W - 10, y + (ROW_H - 2) // 2, (ROW_H - 2) // 2, ui.PANEL))
        art, colors = ICONS[icon]
        g.append(ui.sprite(art, colors, 10 if i == sel else 12, y + (0 if i == sel else 3), scale=2 if i == sel else 1))
        g.append(ui.lbl(name, 32, y + 7, color if i == sel else ui.DIM))
        if i == sel:
            g.append(ui.lbl(tag, ui.W - 10, y + 7, ui.FG, anchor=(1.0, 0.5)))
    px.color(APPS[sel][3])


def twinkle(t):
    for n, s in enumerate(twinkles):
        s.pixel_shader = ui.pal(ui.RAINBOW[(int(t * 3) + n * 2) % len(ui.RAINBOW)])
    if hint:
        text = HINTS[int(t / 2.2) % len(HINTS)]  # cycle through the controls
        if hint[0].text != text:
            hint[0].text = text


def show_error(name, err):
    g = ui.screen()
    ui.header(g, name + " got the hiccups", ui.BAD, "tap: back")
    lines = "".join(traceback.format_exception(err)).split("\n")
    lines = [l.strip() for l in lines if l.strip()][-6:]
    for i, line in enumerate(lines):
        g.append(ui.lbl(ui.clip(line, 39), 4, 28 + i * 17, ui.FG if i == len(lines) - 1 else ui.DIM))
    print("".join(traceback.format_exception(err)))
    px.color(ui.BAD)
    btn.clear()
    while btn.poll() is None:
        time.sleep(0.02)


def run_app(index):
    mod_name, name = APPS[index][0], APPS[index][1]
    full = "apps." + mod_name
    gc.collect()
    ui.state["app"] = mod_name
    try:
        __import__(full)
        sys.modules[full].run(btn, px)
    except Exception as e:  # keep the launcher alive whatever an app does
        show_error(name, e)
    finally:
        ui.state["app"] = "menu"
        sys.modules.pop(full, None)
        gc.collect()
    btn.clear()


sel = microcontroller.nvm[0] if microcontroller.nvm[0] < len(APPS) else 0
draw_menu(sel)
while True:
    e = btn.poll()
    if e == "short":
        sel = (sel + 1) % len(APPS)
        draw_menu(sel)
    elif e == "double":
        sel = (sel - 1) % len(APPS)
        draw_menu(sel)
    elif e == "long":
        if microcontroller.nvm[0] != sel:
            microcontroller.nvm[0] = sel
        run_app(sel)
        draw_menu(sel)
    want = ui.state.pop("launch", None)  # an app picked on the web panel
    if want:
        names = [a[0] for a in APPS]
        if want in names:
            sel = names.index(want)
            draw_menu(sel)
            run_app(sel)
            draw_menu(sel)
    twinkle(time.monotonic())
    time.sleep(0.02)
