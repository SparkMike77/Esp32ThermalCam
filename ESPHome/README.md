# MLX90640 Thermal Camera on TTGO T-Display — ESPHome

A self-contained ESPHome project that reads the Melexis MLX90640 32×24
thermal array and draws a live colour heatmap on the TTGO T-Display's
240×135 ST7789V screen, with a sidebar showing min/max temperatures and
WiFi signal strength.

---

## Hardware

| Part | Notes |
|------|-------|
| LILYGO TTGO T-Display | ESP32 + ST7789V 240×135 TFT |
| Melexis MLX90640 breakout | I2C, 3.3 V; Mabee/M5Stack/SparkFun breakouts all work |

### Wiring

```
MLX90640   →   T-Display
─────────────────────────
VDD        →   3.3 V
GND        →   GND
SDA        →   GPIO21
SCL        →   GPIO22
```

> The T-Display uses GPIO18/19 for SPI (display) and GPIO21/22 are free
> for I2C — these buses don't conflict.

---

## Folder layout

```
thermal_cam_esphome/
├── thermal_camera.yaml            ← ESPHome device config
├── secrets.yaml                   ← WiFi credentials (create yourself)
└── components/
    └── mlx90640_display/
        ├── __init__.py            ← ESPHome component registration
        ├── sensor.py              ← Python schema & code-gen bindings
        └── mlx90640_display.h     ← C++ component implementation
```

---

## secrets.yaml (create next to thermal_camera.yaml)

```yaml
wifi_ssid: "YourSSID"
wifi_password: "YourPassword"
```

---

## Screen layout

```
┌──────────────────────────────────────────┐
│  Thermal image 128×96 px  ║  THERMAL     │
│  (32 cols × 24 rows × 4)  ║  MIN  20.1 C │
│                           ║  MAX  36.8 C │
│         [heat map]        ║  SCALE 15-40 │
│                           ║  WiFi -58dBm │
│  ▓ colour scale bar (4px) ║              │
└──────────────────────────────────────────┘
  ←──── 128 px ────→ 4px ←── 108 px ──→
```

- Scale factor 4 → each MLX90640 pixel = 4×4 screen pixels
- Colour palette: black → blue → cyan → yellow → red (iron/inferno)
- Colour scale bar sits in the 4px gap between image and sidebar
- Image is vertically centred (19px top margin in 135px height)

---

## Tuning parameters

| YAML key | Default | Effect |
|----------|---------|--------|
| `mintemp` | 15 | Colour scale lower bound °C |
| `maxtemp` | 40 | Colour scale upper bound °C |
| `refresh_rate` | 0x04 (8 Hz) | Sensor frame rate |
| `update_interval` | 200 ms | How often ESPHome polls sensor |
| Display `update_interval` | 150 ms | How often screen redraws |
| `SCALE` (in lambda) | 4 | Pixels per thermal pixel (1–5) |

**Changing SCALE to 5** gives a 160×120 px image (slightly outside 135px
height — reduce to keep within bounds, e.g. rows only, or crop).

**To use auto-range** (colour maps to detected min/max each frame),
change the lambda `t_min`/`t_max` to use
`id(thermal_cam).get_min_detected()` and `get_max_detected()`.

---

## Adding more sidebar sensors

Uncomment the `bme280_i2c` section in the YAML to add an ambient
temperature/humidity sensor on the same I2C bus (address 0x76), then
add lines like:

```cpp
it.printf(SB_X + 4, YY, id(font_small), id(C_WHITE),
          "%.1f C", id(ambient_temp).state);
```

---

## Compiling & flashing

```bash
# First flash (USB):
esphome run thermal_camera.yaml

# Subsequent OTA updates:
esphome compile thermal_camera.yaml
esphome upload thermal_camera.yaml --device <ip-address>
```

---

## Colour palette

The `temp_to_color()` static method in `mlx90640_display.h` implements
a 5-stop iron palette.  Adjust the gradient stops to taste:

| Normalised value | Colour |
|---|---|
| 0.00 | Black |
| 0.25 | Blue |
| 0.50 | Cyan |
| 0.75 | Yellow |
| 1.00 | Red/white |
