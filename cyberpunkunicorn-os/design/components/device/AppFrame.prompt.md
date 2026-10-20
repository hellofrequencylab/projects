The standard full-screen app frame (rainbow border, status bar, rainbow title, grey key-hint footer); every app screen starts with it.

```jsx
<DeviceScreen scale={2}>
  <AppFrame title="LIGHT" footer="SEL reset min/max    ESC exit">
    <ScreenText x={14} y={46} size={4}>64%</ScreenText>
  </AppFrame>
</DeviceScreen>
```

Content area: y 46 to ~200, x 10 to 310. Footer copy is key hints only: `KEY action`, keys in caps (SEL, ESC, NEXT/PREV), actions lowercase.
