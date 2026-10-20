Text on a 320x240 device screen, placed in device pixels like `tft.drawString`; use inside AppFrame / DeviceScreen.

```jsx
<ScreenText x={14} y={60} color="#888">Space</ScreenText>
<ScreenText y={46} size={2} align="center" color="var(--cu-cyan)">TAP A TAG</ScreenText>
```

Sizes are GFX steps: 1 = 6x8 px cells (53 chars per line), 2 = 12x16 (26), 3 = 18x24, 4 = 24x32. ASCII only.
