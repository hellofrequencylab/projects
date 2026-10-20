The device main menu: status bar, rainbow-framed panel, one animated app icon with its rainbow name and prev/next arrows; use inside DeviceScreen.

```jsx
<DeviceScreen scale={2}>
  <MenuScreen app="lightshow" index={1} count={23} onNext={next} onPrev={prev} />
</DeviceScreen>
```

Layout approximates Bruce 1.16.1's main menu (icon centred, label under it); the skin colours follow patch 0008.
