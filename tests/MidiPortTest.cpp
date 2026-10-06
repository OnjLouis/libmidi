#include "pch.h"
#include "Exception.h"
#include "IFF.h"
// Exercise the SMF parser directly without pulling unrelated format decoders
// into this standalone regression target or widening the production API.
#define private public
#include "MIDIProcessor.h"
#undef private
#include <set>

namespace
{
using bytes_t = std::vector<uint8_t>;
void Require(bool condition, const char * message)
{
    if (!condition) throw std::runtime_error(message);
}
bytes_t MakeSMF(const std::vector<bytes_t> & tracks)
{
    bytes_t file = {'M','T','h','d',0,0,0,6,0,1,0,static_cast<uint8_t>(tracks.size()),1,0xE0};
    for (const auto & track : tracks)
    {
        file.insert(file.end(), {'M','T','r','k'});
        const auto size = static_cast<uint32_t>(track.size());
        for (int shift = 24; shift >= 0; shift -= 8) file.push_back(static_cast<uint8_t>(size >> shift));
        file.insert(file.end(), track.begin(), track.end());
    }
    return file;
}
std::vector<midi::message_t> Parse(const bytes_t & file, midi::container_t & container,
    midi::sysex_table_t & table, std::vector<uint8_t> & ports)
{
    Require(midi::processor_t::ProcessSMF(file, container), "SMF parsing failed");
    std::vector<midi::message_t> stream;
    uint32_t begin, end;
    container.SerializeAsStream(0, stream, table, ports, begin, end, 0);
    return stream;
}
void TestPorts(bool yamaha)
{
    std::vector<bytes_t> tracks;
    for (uint8_t port = 0; port < 4; ++port)
    {
        bytes_t track = yamaha ? bytes_t{0,0xFF,0x7F,4,0x43,0,1,port} : bytes_t{0,0xFF,0x21,1,port};
        track.insert(track.end(), {0,0xFF,4,1,static_cast<uint8_t>('A' + port)});
        for (uint8_t channel = 0; channel < 16; ++channel)
            track.insert(track.end(), {0,static_cast<uint8_t>(0x90|channel),60,100});
        track.insert(track.end(), {1,0xFF,0x2F,0});
        tracks.push_back(track);
    }
    midi::container_t container; midi::sysex_table_t table; std::vector<uint8_t> ports;
    const auto stream = Parse(MakeSMF(tracks), container, table, ports);
    std::set<uint32_t> pairs;
    for (const auto & event : stream)
        if ((event.Data & 0xF0) == 0x90) pairs.insert(event.Data & 0x7F00000F);
    Require(pairs.size() == 64, "64 independent port/channel pairs were not preserved");
    Require(container.GetChannelCount(0) == 64, "Channel-count metadata folds the fourth port");
    Require(ports == std::vector<uint8_t>({0,1,2,3}), "Port list is incorrect");
    bytes_t roundtrip; container.SerializeAsSMF(roundtrip);
    Require(roundtrip == MakeSMF(tracks), "Port metadata did not round-trip unchanged");
}
void TestChangesAndSysEx()
{
    const bytes_t track = {0,0xFF,0x7F,4,0x43,0,1,0, 0,0x90,60,100,
        1,0xFF,0x7F,4,0x43,0,1,3, 0,0xB0,7,80, 0,0xE0,0,65,
        0,0xF0,5,0x7E,0x7F,9,1,0xF7, 0,0x90,64,100, 1,0xFF,0x2F,0};
    midi::container_t container; midi::sysex_table_t table; std::vector<uint8_t> ports;
    const auto stream = Parse(MakeSMF({track}), container, table, ports);
    Require(ports == std::vector<uint8_t>({0,3}), "Mid-track Yamaha port change was lost");
    Require((stream[0].Data >> 24) == 0, "Earlier event moved to later port");
    for (size_t i : {size_t(1),size_t(2),size_t(4)})
        Require((stream[i].Data >> 24) == 1, "Controller, bend or note reached the wrong normalized port");
    const uint8_t * data; size_t size; uint8_t port;
    Require(table.GetItem(stream[3].Data & 0x7FFFFFFF, data, size, port) && port == 1,
        "SysEx reached the wrong port");
}
void TestUnrelatedMetadata()
{
    for (const bytes_t & marker : {bytes_t{0x43,0,2,3}, bytes_t{0x41,0,1,3},
        bytes_t{0x43,0,1}, bytes_t{0x43,0,1,3,0}, bytes_t{0x43,0,1,0x80}})
    {
        bytes_t track = {0,0xFF,0x7F,static_cast<uint8_t>(marker.size())};
        track.insert(track.end(), marker.begin(), marker.end());
        track.insert(track.end(), {0,0x90,60,100,1,0xFF,0x2F,0});
        midi::container_t container; midi::sysex_table_t table; std::vector<uint8_t> ports;
        Parse(MakeSMF({track}), container, table, ports);
        Require(ports == std::vector<uint8_t>({0}), "Unrelated or malformed metadata changed the port");
    }
}
}

int main(int argc, char ** argv)
{
    try
    {
        if (argc > 1)
        {
            for (int i = 1; i < argc; ++i)
            {
                std::ifstream file(argv[i], std::ios::binary);
                Require(file.good(), "Could not open supplied MIDI file");
                const bytes_t bytes((std::istreambuf_iterator<char>(file)), {});
                midi::container_t container; midi::sysex_table_t table; std::vector<uint8_t> ports;
                const auto stream = Parse(bytes, container, table, ports);
                std::cout << argv[i] << ": tracks=" << container.GetTrackCount()
                    << " channels=" << container.GetChannelCount(0) << " ports=" << ports.size()
                    << " events=" << stream.size() << '\n';
                std::cout << "  port map:";
                for (uint8_t port : ports) std::cout << ' ' << static_cast<unsigned>(port);
                std::cout << '\n';
            }
            return 0;
        }
        TestPorts(true); TestPorts(false); TestChangesAndSysEx(); TestUnrelatedMetadata();
        std::cout << "MIDI port regression tests passed\n";
        return 0;
    }
    catch (const std::exception & error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
