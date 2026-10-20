The on-device options menu: neon-coloured rows in a rainbow box, selected row filled; use for every app's top-level menu and pickers.

```jsx
<AppFrame title="LED">
  <OptionList options={['Colour picker', 'Rainbow cycle', 'Breathe', 'Off']} selected={1} onSelect={go} />
</AppFrame>
```

Options carry an optional `value` shown right-aligned (`{label: 'Loop', value: 'forever'}`). Row colours walk the hue wheel from hot pink, 30° minimum apart.
