# CPS4042 Hardware Simulation

A C++20 project modeling boards, sensors, wires, and communication concepts. The sample in `CA1/src/main.cpp` connects an ESP8266 model to a VL530X sensor model and starts the application event loop.

## Structure

- `CA1/include/CPS4042/Hardwares/`: board, processor, sensor, and transmitter models.
- `CA1/include/CPS4042/Wires/`: links and pins.
- `CA1/include/CPS4042/Units/`: bit, byte, frequency, voltage, and baud-rate types.
- `CA1/include/CPS4042/Sketchs/`: simulation sketches.
- `CA1/src/main.cpp`: example wiring and entry point.

## Build

Requirements: a C++20 compiler, CMake 3.19 or newer, and Boost.

The current `CA1/CMakeLists.txt` contains developer-specific Boost include and library paths. Replace those paths with your local Boost installation before configuring:

```sh
cmake -S CA1 -B build
cmake --build build
```

Run the generated `CPS4042` executable from the build output directory. Its exact location depends on the CMake generator and configuration.

## License

See [LICENSE](LICENSE).
