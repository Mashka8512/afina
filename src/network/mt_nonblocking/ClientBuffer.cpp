#include "ClientBuffer.h"

#include <cstring>

namespace Afina {
namespace Network {
namespace MTnonblock {

ClientBuffer::ClientBuffer() {
    std::memset(buffer, 0, _size);
}

ClientBuffer::~ClientBuffer() {}

char* ClientBuffer::ptr() {
    return buffer;
}

char* ClientBuffer::read_ptr() {
    return buffer + read_offset;
}

char* ClientBuffer::parse_ptr() {
    return buffer + parsed_offset;
}

std::size_t ClientBuffer::size() {
    return _size;
}

std::size_t ClientBuffer::read_size() {
    return _size - read_offset;
}

std::size_t ClientBuffer::parse_size() {
    return _size - read_offset - parsed_offset;
}

void ClientBuffer::read(std::size_t amount) {
    read_offset += amount;
}

void ClientBuffer::parsed(std::size_t amount) {
    parsed_offset += amount;
}

void ClientBuffer::reset() {
    parsed_offset = 0;
    read_offset = 0;
    std::memset(buffer, 0, _size);
}

void ClientBuffer::conditional_reset() {
    if (parse_size() + read_size() < minsize) {
        auto tmp_buffer = char[_size];
        auto tmp_size = parse_size();
        std::memcpy(tmp_buffer, parse_ptr(), tmp_size);
        reset();
        std::memcpy(ptr(), tmp_buffer, tmp_size);
        read_offset = tmp_size;
    }
}

}
}
}
