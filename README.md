# JKvionics — JKLS Flight Computer

Avionics firmware for the **Jack Kenney Launch System (JKLS)** flight computer. Runs on an Arduino-compatible microcontroller and handles real-time altitude tracking, apogee detection, ejection charge firing, and SD card telemetry logging throughout the rocket's flight.

---

## Safety / Project Status

This was an experimental learning project, built as a "figure it out myself" flight computer rather than a certified or flight-proven avionics system. It should be treated as prototype code and hardware.

Important limitations:

- There is **no proper hardware safety interlock** in this design.
- The launch mode button is a software arm / abort input, not an independent pyrotechnic safing system.
- The ejection output can be driven directly by firmware state, so any real flight build should add physical safing, continuity checks, and ground-test procedures before use.
- This system has not been flight-tested as-is.

Anyone revisiting this project should treat the ejection circuit and deployment logic with appropriate caution, follow local rocketry rules, and validate the full system on the bench before considering a launch.

---

## Features

- **Barometric altitude tracking** via BMP388 sensor (80 ms update rate)
- **Automatic apogee detection** using a rolling average of the last 4 altitude measurements
- **Ejection charge firing** at apogee (pin goes HIGH to trigger charge)
- **SD card data logging** — altitude and timestamps written to `data.csv` in real time
- **Post-flight summary** — apogee altitude, time of apogee, and total flight time appended to the log
- **Status LED** — blink patterns indicate initialisation, launch mode, and flight state
- **Software arm / abort button** — arms the system pre-launch; pressing during flight cancels and returns to pre-launch state

---

## Flight State Machine

| State | Description |
|---|---|
| `PRE_LAUNCH` | Idle. Waiting for launch arm button press. |
| `ON_GROUND` | Armed. Timer started, baseline altitude recorded. |
| `ASCENDING` | Vehicle is climbing (avg Δalt > 0.25 m). |
| `DESCENDING` | Vehicle is falling (avg Δalt < −0.25 m). Ejection charge held LOW. |

Apogee is detected when the average altitude change falls between −0.1 m and 0.1 m after ascending. The ejection charge pin (`EJ_PIN`) is driven HIGH at this point.

---

## Hardware

| Component | Details |
|---|---|
| Altitude / Pressure / Temp | BMP388 (I2C) |
| Data storage | SD card module (SPI, CS on pin 10) |
| Launch mode pin | Digital pin 3 (`LM_PIN`) |
| Status LED | Digital pin 4 (`ST_PIN`) |
| Ejection charge | Digital pin 5 (`EJ_PIN`) |

---

## Pin Map

```
D3  — Launch mode / abort button (INPUT)
D4  — Status LED (OUTPUT)
D5  — Ejection charge trigger (OUTPUT)
D10 — SD card chip select (SPI)
SDA/SCL — BMP388 (I2C)
```

---

## Dependencies

Install via the Arduino Library Manager:

- `BMP388_DEV` by Martin Lindupp
- `SD` (built-in)
- `SPI` (built-in)
- `Timer` by Jack Christensen
- `MemoryUsage` by Thierry Paris

---

## Data Log Format

Each flight writes to `data.csv` on the SD card (previous flight data is overwritten on each arm):

```
Begin flight data.
Alt (m),Time (s)
0.00,0.08
1.23,0.16
...
Flight ended.
Time of flight (s):,12.34
Apogee (m):,45.67
Time of apogee (s):,6.78
```

---

## Status LED Behaviour

| Pattern | Meaning |
|---|---|
| 2 quick blinks | Initialisation successful |
| 3 slow blinks → steady ON | Launch mode armed, flight in progress |
| OFF | Pre-launch idle or flight ended |

---

## Project Context

JKLS (Jack Kenney Launch System) is a personal rocketry program under **JKASA**, inspired by large-scale launch systems such as NASA's Space Launch System (SLS). JKvionics is the avionics stack developed for JKLS vehicles.
