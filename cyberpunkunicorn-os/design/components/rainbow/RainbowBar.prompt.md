Progress bar filled with a sweeping rainbow inside a 1px cyan outline; use for loading, writes and scans.

```jsx
<RainbowBar value={0.67} label="Reticulating rainbows..." animate />
<RainbowBar value={1.37} label="All systems fabulous." />
```

The percent is hot pink until 100, then rainbow with "(oops)". Only overshoot on joke loaders, never on real progress.
