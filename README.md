# bite-cpp

`bite-cpp` is a small C++23 serialization library intended to share the same wire-format code between embedded targets such as STM32 and Linux/ROS 2 applications.

The v0.1 core is header-only, zero-allocation, exception-free, RTTI-free, and based on non-owning `std::span` buffers. The wire format is always little endian and is defined field-by-field rather than by copying C++ object representations, so struct padding, alignment, ABI, and host endianness do not define the protocol.

## Requirements

- C++23
- a compiler with C++23 explicit object parameter support (deducing `this`); GCC 14.2 is tested
- C++23 `std::expected`
- 8-bit bytes (`CHAR_BIT == 8`)
- IEEE 754 binary32 `float`
- IEEE 754 binary64 `double`

## Usage

```cpp
#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>

#include <bite/bite.hpp>

struct MotorCommand {
    float velocity;
    float torque;
    std::uint8_t mode;

    constexpr auto bite_fields(this auto& self) noexcept
    {
        return std::tie(self.velocity, self.torque, self.mode);
    }
};

int main()
{
    const MotorCommand command{
        .velocity = 1.0F,
        .torque = 0.5F,
        .mode = 2,
    };

    std::array<std::byte, bite::fixed_encoded_size_v<MotorCommand>> buffer{};

    const auto encoded = bite::encode(command, buffer);
    if (!encoded) {
        return 1;
    }

    const auto decoded = bite::decode<MotorCommand>(buffer);
    if (!decoded) {
        return 1;
    }

    return 0;
}
```

`bite::encode()` returns `std::expected<std::size_t, bite::error>`, where the value is the number of bytes written. For v0.1 fixed-size types, output capacity is checked before any byte is written.

`bite::decode<T>()` returns `std::expected<T, bite::error>`. It requires the top-level input to contain exactly one encoded `T`; remaining bytes produce `bite::error_code::trailing_bytes`.

## User-defined structs

C++23 has no standard reflection, so serializable fields are explicitly listed by a public `bite_fields()` member function using an explicit object parameter:

```cpp
struct Position {
    float x;
    float y;

    constexpr auto bite_fields(this auto& self) noexcept
    {
        return std::tie(self.x, self.y);
    }
};
```

The `this auto& self` form deduces the cv-qualification of the object. Calling `bite_fields()` on a const object during encode therefore returns a tuple of const field references, while calling it on a mutable destination during decode returns mutable field references. One definition handles both cases.

The tuple order returned by `bite_fields()` is the wire order. Nested user-defined structs, arrays, and tuples are handled recursively.

Decoded user-defined types must currently be default-initializable so `bite::decode<T>()` can create the destination object before assigning its listed fields.

## Supported types in v0.1

- signed and unsigned integer types
- `bool` (`false` = `0x00`, `true` = `0x01`; other values are rejected)
- IEEE 754 `float` and `double`
- enums, encoded through their underlying type
- `std::array<T, N>`
- `std::tuple<T...>`
- user-defined structs with a `bite_fields()` member function

All supported v0.1 types have a compile-time wire size:

```cpp
static_assert(bite::fixed_encoded_size_v<std::uint32_t> == 4);
static_assert(bite::fixed_encoded_size_v<MotorCommand> == 9);
```

The size is computed recursively from the wire format and never from `sizeof(struct)`.

## Portable wire schemas

`bite` encodes integral types using their actual width on the target platform. Types such as `int`, `long`, and `unsigned long` may therefore produce different wire formats on different architectures or ABIs.

For protocols shared between embedded targets and desktop/Linux systems, prefer explicitly sized integer types:

```cpp
std::uint8_t
std::uint16_t
std::uint32_t
std::uint64_t
std::int8_t
std::int16_t
std::int32_t
std::int64_t
```

Similarly, enums are encoded using their underlying integer type, which determines their wire width. For portable protocols, explicitly specify the enum's underlying type:

```cpp
enum class Mode : std::uint8_t {
    stop,
    velocity,
    torque,
};
```

Platform-dependent integer types remain supported, but the resulting wire format is portable only between targets where those types have the same width and representation.

## Errors

`bite::error` contains an `error_code` and a byte `offset`:

- `buffer_too_small`
- `unexpected_end`
- `trailing_bytes`
- `invalid_bool`

## CMake

```cmake
add_subdirectory(path/to/bite-cpp)
target_link_libraries(your_target PRIVATE bite::bite)
```

The `bite::bite` interface target requires C++23. The selected compiler must also implement C++23 explicit object parameters and `std::expected`; accepting `-std=c++23` alone is not sufficient.

To build this repository directly:

```sh
cmake -S . -B build -DBITE_BUILD_TESTS=ON -DBITE_BUILD_EXAMPLES=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Current limitations

v0.1 intentionally does not include variable-length or protocol-layer features. In particular, it does not support `std::optional`, `std::variant`, `std::vector`, strings/string views, variable-length sequences, enum-value validation, bit packing, CRC, framing, message IDs, protocol versioning, COBS/SLIP, transport abstractions, ROS/STM32 HAL integration, custom endianness, non-intrusive type registration, or zero-copy decode views.

The core codec is recursive so these can be added later without changing the field-by-field wire-format model.
