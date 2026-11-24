#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static bool live;
static unsigned reads, records, stop_after;
static int read_result;
static char record[256];

static int vermeer_loop_live_ready()
{
    return live;
}

extern "C" int sweet_spmi_read_ppid(unsigned ppid, unsigned offset, uint8_t *value)
{
    static const uint8_t expected_offsets[] = {8, 0x46, 0x46, 0x50, 0x60, 0x57, 0x58, 0x67, 0x68};
    static const uint8_t values[] = {0, 0x80, 0x30, 0x80, 0x80, 0x34, 0x02, 0x34, 0x02};
    unsigned index = reads % 9;
    assert(live && ppid == (index < 2 ? 0x3d8u : 0x3d9u) && offset == expected_offsets[index]);
    *value = values[index];
    if (++reads == stop_after) {
        live = false;
    }
    return read_result;
}

static int sweet_screen_record(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int size = vsnprintf(record, sizeof(record), format, args);
    va_end(args);
    assert(size > 0 && static_cast<unsigned>(size) < sizeof(record));
    ++records;
    return 1;
}

#include "LilacWledScreenProbe.h"

int main()
{
    const char *token = "0123456789abcdef";
    live = true;
    lilac_wled_screen_probe(token, 1);
    assert(reads == 18 && records == 1);
    assert(!strcmp(record, "SWEET WLED SAMPLE token=0123456789abcdef id=1 state=6 fault=0 "
                           "enabled=1 sinks=3 modulators=3 duty0=564 duty1=564 read_only=1"));
    const unsigned before_reads = reads;
    const unsigned before_records = records;
    lilac_wled_screen_probe(token, 2);
    live = false;
    lilac_wled_screen_probe(token, 1);
    assert(reads == before_reads && records == before_records);
    for (unsigned stop = 1; stop <= 18; ++stop) {
        live = true;
        reads = 0;
        stop_after = stop;
        lilac_wled_screen_probe(token, 1);
        assert(reads == stop && records == before_records);
    }
    live = true;
    reads = stop_after = 0;
    lilac_wled_screen_probe(token, 1);
    assert(reads == 18 && records == before_records + 1);
    assert(!strcmp(record, "SWEET WLED SAMPLE token=0123456789abcdef id=1 state=6 fault=0 "
                           "enabled=1 sinks=3 modulators=3 duty0=564 duty1=564 read_only=1"));
    static const unsigned allowed[][2] = {{0x3d8,0x08},{0x3d8,0x46},{0x3d9,0x46},{0x3d9,0x50},
        {0x3d9,0x60},{0x3d9,0x57},{0x3d9,0x58},{0x3d9,0x67},{0x3d9,0x68}};
    uint8_t observed = 0;
    reads = 0;
    for (unsigned index = 0; index < 9; ++index) {
        live = true;
        assert(lilac_wled_screen_read(nullptr, allowed[index][0], allowed[index][1], &observed) == 0);
    }
    assert(reads == 9);
    assert(lilac_wled_screen_read(nullptr, 0x3d8, 0x47, &observed) == -1);
    assert(lilac_wled_screen_read(nullptr, 0x3d9, 0x51, &observed) == -1);
    assert(lilac_wled_screen_read(nullptr, 0x3c8, 0x08, &observed) == -1);
    assert(lilac_wled_screen_read(nullptr, 0x3d8, 0x08, nullptr) == -1);
    assert(reads == 9);
    live = false;
    assert(lilac_wled_screen_read(nullptr, 0x3d8, 0x08, &observed) == -1);
    live = true;
    read_result = 0;
    reads = 0;
    stop_after = 1;
    lilac_wled_screen_probe(token, 1);
    assert(!live && reads == 1 && records == before_records + 1);
    live = true;
    reads = stop_after = 0;
    read_result = -1;
    lilac_wled_screen_probe(token, 1);
    assert(records == before_records + 2 && reads == 1);
    assert(strstr(record, "state=0 fault=0 enabled=0 sinks=0 modulators=0 duty0=0 duty1=0"));
    puts("PASS: first-frame WLED probe checks allowed addresses, loss of LIVE, and read failure");
}
