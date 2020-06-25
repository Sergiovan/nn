#include "common/util.h"

ss::streamstring ss::get() {
    return {};
}

ss::streamstring_end ss::end() {
    return {};
}

u64 parse_hex(const char* c, u8 max_len, u8* len) {
    u8 _len;
    if (!len) {
        len = &_len;
    }
    *len = 0;
    u64 value{0};
    for (; c && *len < max_len; ++c) {
        switch (*c) {
            case '0': [[fallthrough]];
            case '1': [[fallthrough]];
            case '2': [[fallthrough]];
            case '3': [[fallthrough]];
            case '4': [[fallthrough]];
            case '5': [[fallthrough]];
            case '6': [[fallthrough]];
            case '7': [[fallthrough]];
            case '8': [[fallthrough]];
            case '9':
                value = (value << 4) | ((*c - '0') & 0xF);
                break;
            case 'a': [[fallthrough]];
            case 'b': [[fallthrough]];
            case 'c': [[fallthrough]];
            case 'd': [[fallthrough]];
            case 'e': [[fallthrough]];
            case 'f':
                value = (value << 4) | (((*c - 'a') + 10) & 0xF);
                break;
            case 'A': [[fallthrough]];
            case 'B': [[fallthrough]];
            case 'C': [[fallthrough]];
            case 'D': [[fallthrough]];
            case 'E': [[fallthrough]];
            case 'F':
                value = (value << 4) | (((*c - 'A') + 10) & 0xF);
                break;
        }
    }
    
    return value;
}

constexpr std::array<u8, 9> utf8_length{{1, 0, 2, 3, 4, 0, 0, 0, 0}};
constexpr std::array<u8, 4> utf8_mask{{0xFF, 0x1F, 0xF, 0x7}};

u64 utf8_to_utf32(const char* c, u8* plen) {
    u8 rlen{0};
    u64 value = read_utf8(c, rlen, plen);
    if (value < 0xFFFFFFFF || !rlen) {
        return value;
    }
    
    switch (rlen) {
        case 1:
            value = value & 0xFF;
            break;
        case 2:
            value = (value & 0xFF) << 6 | (value & 0xFF00);
            break;
        case 3:
            value = (value & 0xFF) << 12 | (value & 0xFF00) << 6 | (value & 0xFF0000);
            break;
        case 4:
            value = (value & 0xFF) << 18 | (value & 0xFF00) << 12 | (value & 0xFF0000) << 6 | (value & 0xFF000000);
            break;
        default:
            ASSERT(false, "Impossible!");
    }
    
    return value;
}

u64 read_utf8(const char* c, u8& bytes, u8* plen) {
    u8 _len;
    if (!plen) {
        plen = &_len;
    }
    *plen = 0;
    u32 value{0};
    
    if (c[0] == '\\') {
        switch (c[1]) {
            case '\\':
                value = '\\';
                break;
            case 'n':
                value = '\n';
                break;
            case 't':
                value = '\t';
                break;
            case 'u': {
                u8 parsed{0};
                value = parse_hex(c + 2, 6, &parsed);
                *plen += parsed;
                if (parsed != 6) {
                    return value | ((u64) *plen << 32);
                }
                break;
            }
            case 'x': 
                goto utf_escape;
            default: 
                value = c[1];
                break;
        }
        *plen = 2;
        return value;
    }
    
    utf_escape:
    
    constexpr std::array<u8, 9> utf8_length{{1, 0, 2, 3, 4, 0, 0, 0, 0}};
    constexpr std::array<u8, 4> utf8_mask{{0xFF, 0x1F, 0xF, 0x7}};
    
    u8 chars[4] {0, 0, 0, 0};
    u8 len = 0;
    u8 rlen = 0;
    const char* start = c;
    
    if (*c == '\\') {
        if (c[1] != 'x') {
            return (((c - start) + 1) << 32);
        }
        u8 parsed{0};
        chars[len] =  (u8) parse_hex(c + 2, 2, &parsed);
        c += parsed + 2;
        *plen = c - start;
        if (parsed != 2) {
            return ((u64)*plen << 32);
        }
        
    } else {
        chars[len] = *c;
        c++;
        ++*plen;
    }
    
    u8 ones = __builtin_clz(~chars[0]);
    bytes = rlen = utf8_length[ones];
    chars[0] &= utf8_mask[rlen - 1];
    
    len++;
    while (c && len < rlen) {
        if (*c == '\\') {
            if (c[1] != 'x') {
                return (((c - start) + 1) << 32);
            }
            u8 parsed{0};
            chars[len] =  (u8) parse_hex(c + 2, 2, &parsed);
            c += parsed + 2;
            *plen = c - start;
            if (parsed != 2) {
                return ((u64)*plen << 32);
            }
            
        } else {
            chars[len] = *c;
            c++;
            ++*plen;
        }
        
        if (chars[len] < 0x70) {
            return ((c - start) << 32);
        }
        chars[len] &= 0x3F;
        
        len++;
    }
    
    if (len < rlen) {
        return ((c - start) << 32);
    }
    
    return *reinterpret_cast<u32*>(chars);
}

u64 align(u64 offset, u64 to) {
    ASSERT(to > 0, "Invalid alignment");
    constexpr std::array<u8, 9> ceil_2{{1, 1, 2, 4, 4, 8, 8, 8, 8}};
    to = to > 8 ? 8 : to;            // 0  1  2  3  4  5  6  7  8 
    to = ceil_2[to];
    u8 rem = offset & (to - 1); // Works because power of 2
    return offset + ((to - rem) & ~((rem == 0) - 1));
}
