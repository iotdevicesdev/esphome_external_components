# ESPHome external components
# - ggreg20_v3 - GGreg20_V3 Geiger counter ready to use ESPHome external component
This is an **external ESPHome component** for the DIY Geiger Counter Module **GGreg20_V3** by **IoT-devices LLC**.

The component simplifies the integration of the GGreg20\_V3 module with any ESP-based device running ESPHome. It moves complex logic—including pulse counting, dead time filtering, dose calculation, and status determination—from YAML into reliable C++.

> Please note that this component needs its own GPIO and cannot coexist with `pulse_counter` on the same pin. You usually don't need to know this, since the `pulse_counter` component and the external `ggreg20_v3` component are alternatives to each other—unless you're trying to include both in your ESPHome YAML file at the same time and compile it for your own purposes, such as testing. ESPHome's 'pulse_counter' claims the pin for the ESP32 PCNT peripheral, after which the GPIO interrupt of external `ggreg20_v3` component no longer fires.

> ESPHome also rejects `allow_other_uses: true` once a pin is used only once. Please note this as well.

 
---
<img width="1105" height="359" alt="image" src="https://github.com/user-attachments/assets/0668fc14-00b6-4000-8b41-ed88c4dfc68e" />

## 🚀 Features

The component automatically calculates and provides **six** essential sensors in Home Assistant:

1.  **Radiation Power ($\mu\text{Sv/h}$):** Ionizing radiation power.
2.  **Equivalent Dose ($\mu\text{Sv/h}$):** Equivalent dose absorbed by the human body.
3.  **Total Accumulated Dose ($\mu\text{Sv}$):** Total integrated dose over time.
4.  **Radiation Counts Per Minute ($\text{CPM}$):** Calculated pulse rate per measurement period.
5.  **Instant Pulse Count ($\text{count}$):** Raw, instant pulse count.
6.  **Status (Text Sensor):** Operational status and warning level.

## 🛠 Installation

This component is distributed as an ESPHome **External Component**.

### Step 1: Add External Component

In your $\text{ESPHome}$ YAML file, add the `external_components` section, pointing to this GitHub repository.

### Step 2: Configure the GGreg20_V3 Module
Add the main configuration block, defining your pin, measurement parameters, and desired sensors. All $\text{\\_id}$ sections are mandatory.

## 📋 Configuration Variables

### Main Configuration (`ggreg20_v3:`)

| Name | Type | Default | Description |
| :--- | :--- | :--- | :--- |
| **`pin`** | [**Pin**](https://esphome.io/guides/configuration-api.html#config-pin) | **REQUIRED** | The GPIO pin the pulse output is connected to. |
| `int_logic` | integer | `0` | Sets the Interrupt (ISR) processing logic: 0 = Active-Low (default), 1 = Active-High |
| `measurement_period` | integer | `60` | The interval, in seconds, over which pulses are counted to calculate $\text{CPM}$. |
| `dead_time_us` | integer | `190` | The minimum time between two pulses for a signal to be valid, in microseconds. This implements dead time correction. |
| `dose_power_factor` | float | `0.00378` | Conversion factor for Ionizing Radiation Power ($\mu\text{Sv/h}$). |
| `equivalent_dose_factor` | float | `0.00332` | Conversion factor for Equivalent Dose ($\mu\text{Sv/h}$). |

### Sensor IDs

The following configuration blocks **must** be provided to define the sensor entities.

| Name | Type | Description |
| :--- | :--- | :--- |
| **`radiation_power`** | [**Sensor**](https://esphome.io/components/sensor/index.html) | Configuration for the Raiation Power ($\mu\text{Sv/h}$) sensor. |
| **`equivalent_dose`**| [**Sensor**](https://esphome.io/components/sensor/index.html) | Configuration for the Equivalent Dose ($\mu\text{Sv/h}$) sensor. |
| **`total_dose`** | [**Sensor**](https://esphome.io/components/sensor/index.html) | Configuration for the Total Accumulated Dose ($\mu\text{Sv}$) sensor (calculated internally via integration). |
| **`cpm`** | [**Sensor**](https://esphome.io/components/sensor/index.html) | Configuration for the Counts Per Minute ($\text{CPM}$) per measurement cycle sensor. |
| **`count`** | [**Sensor**](https://esphome.io/components/sensor/index.html) | Configuration for the Instant Pulse Count ($\text{COUNT}$) sensor. |
| **`status`** | [**Text Sensor**](https://esphome.io/components/text_sensor/index.html) | Configuration for the System Status text sensor. |

---

## 🚦 Status Logic

The **Status** sensor is a Text Sensor that reports the current operational state based on the calculated $\text{CPM}$ and Radiation Power values:

| Condition | Status | Notes |
| :--- | :--- | :--- |
| $\text{CPM} > 315K$ | **Sensor Overflow Error** | Radiation level exceeds the GM tube's capacity. |
| $0.6 < \text{Rad.Power} \leq 315K \text{ CPM}$ | **Danger** | High radiation level. |
| $0.3 < \text{Rad.Power} \leq 0.6 \mu\text{Sv/h}$ | **Warning** | Elevated radiation level. |
| $3 \leq \text{CPM} \leq 0.3 \mu\text{Sv/h}$ | **Normal** | Safe or typical background level. |
| $\text{CPM} < 3$ | **Sensor Error (Low Count)** | Possible sensor disconnection or tube malfunction (too few pulses received). |

---

## ✨ Advanced Configuration (Optional)

You can use standard $\text{ESPHome}$ filters on the output sensors. For example, to apply a 5-minute Moving Average filter to the Radiation Power sensor, use the $\text{filter}$ platform:

```yaml
sensor:
  - platform: filter
    name: "GGreg20 Radiation Power MA5"
    # Source ID combines the Component ID and the sensor's normalized key/name:
    source_id: ggreg_radiation_power  
    filters:
      - sliding_window_moving_average:
          window_size: 5
          send_every: 1
```

## YAML Examples
### Local external component config

<img width="1174" height="612" alt="image" src="https://github.com/user-attachments/assets/4247279d-252e-4596-9a03-440f4941e018" />

#### YAML-config for local setup
```yaml
esphome:
  name: sensor-node

external_components:
  - source:
      type: local
      path: ./my_components/
    components: [ggreg20_v3]

ggreg20_v3:
  id: ggreg
  
  # Pin configuration for Active-Low operation (default)
  pin:
    number: GPIO4
    mode: INPUT      # No pullup - GGreg20_V3 manages output
    inverted: true   # Read LOW as logical "1" (active state)
  
  # Interrupt logic: 0 = Active-Low (default), 1 = Active-High
  int_logic: 0
  
  # Measurement settings
  measurement_period: 60    # seconds (minimum 10)
  dead_time_us: 190        # microseconds (0-1000)
  
  # Calibration factors (adjust for your specific sensor)
  dose_power_factor: 0.00378     # CPM to µSv/h conversion for J305
  equivalent_dose_factor: 0.00332 # CPM to equivalent µSv/h conversion for J305

  # Sensor definitions (All IDs are mandatory)
  radiation_power:
    name: "Radiation Power"
  equivalent_dose:
    name: "Radiation Equivalent Dose"
  total_dose:
    name: "Total Accumulated Dose"
  cpm:
    name: "Radiation CPM"
  count:
    name: "Pulse Count"
  status:
    name: "Sensor Status"
```


### Git external component config
#### YAML for GitHub setup
```yaml
esphome:
  name: sensor-node

external_components:
  - source:
      type: git
      url: https://github.com/iotdevicesdev/esphome_external_components 
      ref: main 
    components: [ggreg20_v3] 

ggreg20_v3:
  id: ggreg
  
  # Pin configuration for Active-Low operation (default)
  pin:
    number: GPIO4
    mode: INPUT      # No pullup - GGreg20_V3 manages output
    inverted: true   # Read LOW as logical "1" (active state)
  
  # Interrupt logic: 0 = Active-Low (default), 1 = Active-High
  int_logic: 0
  
  # Measurement settings
  measurement_period: 60    # seconds (minimum 10)
  dead_time_us: 190        # microseconds (0-1000)
  
  # Calibration factors (adjust for your specific sensor)
  dose_power_factor: 0.00378     # CPM to µSv/h conversion for J305
  equivalent_dose_factor: 0.00332 # CPM to equivalent µSv/h conversion for J305

  # Sensor definitions (All IDs are mandatory)
  radiation_power:
    name: "Radiation Power"
  equivalent_dose:
    name: "Radiation Equivalent Dose"
  total_dose:
    name: "Total Accumulated Dose"
  cpm:
    name: "Radiation CPM"
  count:
    name: "Pulse Count"
  status:
    name: "Sensor Status"
```
### Resulting GGreg20_V3 entities in HA 
<img width="1242" height="665" alt="image" src="https://github.com/user-attachments/assets/f67d3c14-e21e-4359-b65f-ef996bd53259" />

## Links
[GGreg20_V3 Product](https://iot-devices.com.ua/en/product/ggreg20_v3-ionizing-radiation-detector-with-geiger-tube-sbm-20/)

[Geiger counter GGreg20_V3: maximum radiation that can be measured](https://iot-devices.com.ua/en/maximum-radiation-that-can-be-measured-by-geiger-counter-ggreg20_v3-en/)

[Geiger tube J305 conversion factor: differences between the coefficient for source radiation power and absorbed dose. Technical note](https://iot-devices.com.ua/en/geiger-tube-j305-conversion-factor-difference-for-radiation-source-power-and-absorbed-dose-technical-note-en/)

[Geiger tube J305: How to calculate the conversion factor of CPM to μSv/h Technical note](https://iot-devices.com.ua/en/geiger-tube-j305-how-to-calculate-the-conversion-factor-of-cpm-technical-note-en/)
