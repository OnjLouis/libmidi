cmake_minimum_required(VERSION 3.20)
project(midi_port_regression LANGUAGES CXX)
enable_testing()
# Compile the unchanged production VLQ decoder without linking every other
# supported file format into this deliberately SMF-only regression target.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../src/MIDIProcessor.cpp" processor_source)
string(FIND "${processor_source}" "int processor_t::DecodeVariableLengthQuantity(" decoder_start)
if(decoder_start EQUAL -1)
    message(FATAL_ERROR "Production VLQ decoder not found")
endif()
string(SUBSTRING "${processor_source}" ${decoder_start} -1 decoder_source)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/SMFProcessorSupport.cpp"
    "#include \"MIDIProcessor.h\"\nnamespace midi {\nprocessor_options_t processor_t::_Options;\n${decoder_source}")
add_executable(midi-port-test MidiPortTest.cpp
    ../src/MIDIContainer.cpp "${CMAKE_CURRENT_BINARY_DIR}/SMFProcessorSupport.cpp"
    ../src/MIDIProcessorSMF.cpp ../src/SysEx.cpp ../src/Tables.cpp)
target_compile_features(midi-port-test PRIVATE cxx_std_20)
target_include_directories(midi-port-test PRIVATE ../src ../include support)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND NOT MSVC)
    target_compile_options(midi-port-test PRIVATE -ffunction-sections -fdata-sections -Wno-unknown-pragmas)
    target_link_options(midi-port-test PRIVATE -Wl,--gc-sections -static)
endif()
add_test(NAME midi-port-regression COMMAND midi-port-test)
