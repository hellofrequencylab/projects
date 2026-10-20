Yellow CYD bezel around a 320x240 device-pixel screen; use to present on-device screens on the web (docs, console mirror).

```jsx
<DeviceScreen scale={2}><BootScene /></DeviceScreen>
<DeviceScreen bezel={false} scale={3}><MenuScreen app="hat" /></DeviceScreen>
```

Children are laid out at device pixels; never use fractional `scale`.
