# MIDI port regression tests

These Windows tests compile the production Standard MIDI File parser and
container. Synthetic fixtures require no copyrighted MIDI songs or ROM files.
They exercise standard and Yamaha four-port routing, all 64 port/channel pairs,
explicit port precedence over instrument names, timed port changes, SysEx,
invalid manufacturer metadata, and byte-for-byte SMF round trips.

Configure with CMake 3.20 or later and a C++20 compiler, keeping the build
directory outside the source checkout:

```text
cmake -S tests -B ../midi-port-build
cmake --build ../midi-port-build --config Release
ctest --test-dir ../midi-port-build -C Release --output-on-failure
```

The target extracts the unchanged VLQ decoder from MIDIProcessor.cpp at
configure time. Small test-only support headers replace unrelated libmsc and
MSVC analysis dependencies. This is an SMF routing test, not a build or test of
every supported legacy MIDI format.

The test executable also accepts MIDI paths to print track, channel and port
counts. It does not change those files or render audio.
