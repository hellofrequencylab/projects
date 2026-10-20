Animated pixel-art app icon for any CyberUnicorn 5000 app or Bruce menu; use wherever an app is represented (menus, launchers, web console cards).

```jsx
<PixelIcon name="frequency" />            // on-device size, 96x72, animated
<PixelIcon name="lightshow" cell={6} />    // 2x for web hero use
<PixelIcon name="hat" animate={false} frame={0} />
```

- `cell` must be an integer: icons are pixel art, never scale fractionally.
- 23 ids: frequency, lightshow, sensors, led, light, sound, slideshow, weather, hat, gpio, remote, wifi, ble, rf, rfid, ir, nrf24, gps, scripts, others, clock, connect, config.
- Designed for a black background (the menu's `bgColor`). The K outline vanishes on black by design, as on the boot unicorn.
