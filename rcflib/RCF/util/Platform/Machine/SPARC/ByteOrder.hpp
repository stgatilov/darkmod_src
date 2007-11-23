
// Machine-specific definitions - SPARC

#ifndef INCLUDE_UTIL_PLATFORM_MACHINE_SPARC_BYTEORDER_HPP
#define INCLUDE_UTIL_PLATFORM_MACHINE_SPARC_BYTEORDER_HPP

namespace Platform {

    namespace Machine {

        class BigEndian {};
        class LittleEndian {};
        typedef BigEndian ByteOrder;

    } // namespace Machine

} // namespace Platform

#endif // ! INCLUDE_UTIL_PLATFORM_MACHINE_SPARC_BYTEORDER_HPP
