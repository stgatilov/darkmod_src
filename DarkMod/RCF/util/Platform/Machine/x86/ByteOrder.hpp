
// Machine-specific definitions - x86

#ifndef INCLUDE_UTIL_PLATFORM_MACHINE_X86_BYTEORDER_HPP
#define INCLUDE_UTIL_PLATFORM_MACHINE_X86_BYTEORDER_HPP

namespace Platform {

    namespace Machine {

        class BigEndian {};
        class LittleEndian {};
        typedef LittleEndian ByteOrder;

    } // namespace Machine

} // namespace Platform

#endif // ! INCLUDE_UTIL_PLATFORM_MACHINE_X86_BYTEORDER_HPP
