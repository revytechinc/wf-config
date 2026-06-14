# wf-config

Dynamic file-based configuration library for [Wayfire](https://github.com/WayfireWM/wayfire).

## Features

- **Typed Options**: Supports a wide range of option types:
  - Basic types: `int`, `bool`, `double`, `std::string`
  - Colors (`#RRGGBBAA` format)
  - Key bindings (`<super> <alt> KEY_E`)
  - Button bindings
  - Touch gestures (swipe, pinch, edge swipe)
  - Hotspot bindings
  - Output modes and positions
  - Compound options (groups of typed tuples)

- **File-based Configuration**:
  - Read/write INI-style text files
  - Read XML definition files for option schemas
  - Hierarchical configuration (system-wide and per-user)

- **Change Notifications**: Register callbacks to be notified when option values change

- **Bounds Checking**: Arithmetic options can have min/max constraints with automatic clamping

## Building

```bash
meson setup build
ninja -C build
```

### Dependencies

- `meson` >= 0.47.0
- C++17 compiler
- `libevdev`
- `libxml-2.0`
- `glm`

### Options

- `tests` (default: `auto`) - Build unit tests
- `locale_test` (default: `false`) - Test number locale conversions

## Usage

```cpp
#include <wayfire/config/config-manager.hpp>
#include <wayfire/config/file.hpp>

// Create config manager
wf::config::config_manager_t config;

// Load options from XML definitions
// (typically done by plugins via XML)

// Load user/system config files
load_configuration_options_from_file(config, "/path/to/config.ini");

// Get an option value
auto opt = config.get_option<int>("section/option_name");
if (opt) {
    int value = opt->get_value();
}

// Register for changes
opt->add_updated_handler(&callback);
```

### Config File Format

```ini
[section_name]
option1 = value1
option2 = value2
```

### XML Option Definitions

```xml
<wayfire>
  <section name="my_section">
    <option name="my_option" type="int" default="42">
      <min>0</min>
      <max>100</max>
    </option>
  </section>
</wayfire>
```

## License

MIT
