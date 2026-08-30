#pragma once

#include <array>
#include <bit>
#include <climits>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <limits>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

#include <bite/error.hpp>
#include <bite/reader.hpp>
#include <bite/writer.hpp>

namespace bite {
namespace detail {

static_assert(CHAR_BIT == 8, "bite requires 8-bit bytes");

template <class>
inline constexpr bool always_false_v = false;

template <class T>
using unqualified_t = std::remove_cvref_t<T>;

template <class T>
concept has_bite_fields = requires(T& value) {
    bite_fields(value);
    requires noexcept(bite_fields(value));
};

template <class T>
using bite_fields_t = decltype(bite_fields(std::declval<T&>()));

template <class T>
struct is_std_array : std::false_type {};

template <class T, std::size_t N>
struct is_std_array<std::array<T, N>> : std::true_type {};

template <class T>
inline constexpr bool is_std_array_v = is_std_array<unqualified_t<T>>::value;

template <class T>
struct array_traits;

template <class T, std::size_t N>
struct array_traits<std::array<T, N>> {
    using value_type = T;
    static constexpr std::size_t size = N;
};

template <class T>
struct is_std_tuple : std::false_type {};

template <class... T>
struct is_std_tuple<std::tuple<T...>> : std::true_type {};

template <class T>
inline constexpr bool is_std_tuple_v = is_std_tuple<unqualified_t<T>>::value;

template <class T>
concept supported_integer = std::integral<unqualified_t<T>> && !std::same_as<unqualified_t<T>, bool>;

template <class T>
consteval std::size_t fixed_encoded_size();

template <class Tuple, std::size_t... I>
consteval std::size_t tuple_fixed_encoded_size(std::index_sequence<I...>)
{
    return (fixed_encoded_size<std::tuple_element_t<I, Tuple>>() + ... + std::size_t{0});
}

template <class T>
consteval std::size_t fixed_encoded_size()
{
    using U = unqualified_t<T>;

    if constexpr (std::same_as<U, bool>) {
        return 1;
    } else if constexpr (supported_integer<U>) {
        constexpr auto wire_bits = sizeof(U) * CHAR_BIT;
        if constexpr (std::is_signed_v<U>) {
            static_assert(
                std::numeric_limits<U>::digits + 1 == wire_bits,
                "bite requires integer types without padding bits");
        } else {
            static_assert(
                std::numeric_limits<U>::digits == wire_bits,
                "bite requires integer types without padding bits");
        }
        return sizeof(U);
    } else if constexpr (std::same_as<U, float>) {
        static_assert(sizeof(float) == 4, "bite requires 32-bit float");
        static_assert(std::numeric_limits<float>::is_iec559, "bite requires IEEE 754 float");
        return 4;
    } else if constexpr (std::same_as<U, double>) {
        static_assert(sizeof(double) == 8, "bite requires 64-bit double");
        static_assert(std::numeric_limits<double>::is_iec559, "bite requires IEEE 754 double");
        return 8;
    } else if constexpr (std::is_enum_v<U>) {
        return fixed_encoded_size<std::underlying_type_t<U>>();
    } else if constexpr (is_std_array_v<U>) {
        using traits = array_traits<U>;
        return traits::size * fixed_encoded_size<typename traits::value_type>();
    } else if constexpr (is_std_tuple_v<U>) {
        return tuple_fixed_encoded_size<U>(std::make_index_sequence<std::tuple_size_v<U>>{});
    } else if constexpr (has_bite_fields<U>) {
        using fields_type = unqualified_t<bite_fields_t<U>>;
        static_assert(is_std_tuple_v<fields_type>, "bite_fields() must return std::tuple");
        return fixed_encoded_size<fields_type>();
    } else {
        static_assert(always_false_v<U>, "bite: unsupported type");
        return 0;
    }
}

template <class T>
[[nodiscard]] constexpr std::expected<void, error> encode_value(const T& value, writer& output) noexcept;

template <class T>
[[nodiscard]] constexpr std::expected<void, error> decode_value(T& value, reader& input) noexcept;

template <supported_integer T>
[[nodiscard]] constexpr std::expected<void, error> encode_integer(T value, writer& output) noexcept
{
    using U = unqualified_t<T>;
    using unsigned_type = std::make_unsigned_t<U>;
    using shift_type =
        std::conditional_t<(sizeof(unsigned_type) < sizeof(unsigned int)), unsigned int, unsigned_type>;
    const auto bits = static_cast<shift_type>(static_cast<unsigned_type>(value));

    std::array<std::byte, sizeof(U)> bytes{};
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const auto shifted = bits >> (i * 8);
        bytes[i] = static_cast<std::byte>(shifted & static_cast<shift_type>(0xffu));
    }
    return output.write(bytes);
}

template <supported_integer T>
[[nodiscard]] constexpr std::expected<void, error> decode_integer(T& value, reader& input) noexcept
{
    using U = unqualified_t<T>;
    using unsigned_type = std::make_unsigned_t<U>;

    const auto bytes = input.read(sizeof(U));
    if (!bytes) {
        return std::unexpected(bytes.error());
    }

    unsigned_type bits{};
    for (std::size_t i = 0; i < bytes->size(); ++i) {
        const auto byte = static_cast<unsigned_type>(std::to_integer<unsigned int>((*bytes)[i]));
        bits |= static_cast<unsigned_type>(byte << (i * 8));
    }
    value = static_cast<U>(bits);
    return {};
}

template <class Tuple>
[[nodiscard]] constexpr std::expected<void, error> encode_tuple(const Tuple& value, writer& output) noexcept
{
    std::expected<void, error> result{};
    std::apply(
        [&](const auto&... element) {
            ([&] {
                if (result) {
                    result = encode_value(element, output);
                }
            }(),
             ...);
        },
        value);
    return result;
}

template <class Tuple>
[[nodiscard]] constexpr std::expected<void, error> decode_tuple(Tuple& value, reader& input) noexcept
{
    std::expected<void, error> result{};
    std::apply(
        [&](auto&... element) {
            ([&] {
                if (result) {
                    result = decode_value(element, input);
                }
            }(),
             ...);
        },
        value);
    return result;
}

template <class T>
[[nodiscard]] constexpr std::expected<void, error> encode_value(const T& value, writer& output) noexcept
{
    using U = unqualified_t<T>;

    if constexpr (std::same_as<U, bool>) {
        const std::array<std::byte, 1> byte{value ? std::byte{0x01} : std::byte{0x00}};
        return output.write(byte);
    } else if constexpr (supported_integer<U>) {
        return encode_integer(value, output);
    } else if constexpr (std::same_as<U, float>) {
        static_assert(sizeof(float) == sizeof(std::uint32_t));
        static_assert(std::numeric_limits<float>::is_iec559);
        return encode_integer(std::bit_cast<std::uint32_t>(value), output);
    } else if constexpr (std::same_as<U, double>) {
        static_assert(sizeof(double) == sizeof(std::uint64_t));
        static_assert(std::numeric_limits<double>::is_iec559);
        return encode_integer(std::bit_cast<std::uint64_t>(value), output);
    } else if constexpr (std::is_enum_v<U>) {
        return encode_integer(std::to_underlying(value), output);
    } else if constexpr (is_std_array_v<U>) {
        std::expected<void, error> result{};
        for (const auto& element : value) {
            if (!result) {
                break;
            }
            result = encode_value(element, output);
        }
        return result;
    } else if constexpr (is_std_tuple_v<U>) {
        return encode_tuple(value, output);
    } else if constexpr (has_bite_fields<const U>) {
        auto fields = bite_fields(value);
        static_assert(is_std_tuple_v<decltype(fields)>, "bite_fields() must return std::tuple");
        return encode_tuple(fields, output);
    } else {
        static_assert(always_false_v<U>, "bite: unsupported type for encode");
        return {};
    }
}

template <class T>
[[nodiscard]] constexpr std::expected<void, error> decode_value(T& value, reader& input) noexcept
{
    using U = unqualified_t<T>;

    if constexpr (std::same_as<U, bool>) {
        const auto byte = input.read(1);
        if (!byte) {
            return std::unexpected(byte.error());
        }

        const auto raw = std::to_integer<unsigned int>((*byte)[0]);
        if (raw > 1u) {
            return std::unexpected(error{error_code::invalid_bool, input.offset() - 1});
        }
        value = raw != 0u;
        return {};
    } else if constexpr (supported_integer<U>) {
        return decode_integer(value, input);
    } else if constexpr (std::same_as<U, float>) {
        static_assert(sizeof(float) == sizeof(std::uint32_t));
        static_assert(std::numeric_limits<float>::is_iec559);
        std::uint32_t bits{};
        const auto result = decode_integer(bits, input);
        if (!result) {
            return result;
        }
        value = std::bit_cast<float>(bits);
        return {};
    } else if constexpr (std::same_as<U, double>) {
        static_assert(sizeof(double) == sizeof(std::uint64_t));
        static_assert(std::numeric_limits<double>::is_iec559);
        std::uint64_t bits{};
        const auto result = decode_integer(bits, input);
        if (!result) {
            return result;
        }
        value = std::bit_cast<double>(bits);
        return {};
    } else if constexpr (std::is_enum_v<U>) {
        std::underlying_type_t<U> underlying{};
        const auto result = decode_integer(underlying, input);
        if (!result) {
            return result;
        }
        value = static_cast<U>(underlying);
        return {};
    } else if constexpr (is_std_array_v<U>) {
        std::expected<void, error> result{};
        for (auto& element : value) {
            if (!result) {
                break;
            }
            result = decode_value(element, input);
        }
        return result;
    } else if constexpr (is_std_tuple_v<U>) {
        return decode_tuple(value, input);
    } else if constexpr (has_bite_fields<U>) {
        auto fields = bite_fields(value);
        static_assert(is_std_tuple_v<decltype(fields)>, "bite_fields() must return std::tuple");
        return decode_tuple(fields, input);
    } else {
        static_assert(always_false_v<U>, "bite: unsupported type for decode");
        return {};
    }
}

}  // namespace detail

template <class T>
inline constexpr std::size_t fixed_encoded_size_v = detail::fixed_encoded_size<T>();

template <class T>
[[nodiscard]] constexpr std::expected<std::size_t, error> encode(
    const T& value, std::span<std::byte> output) noexcept
{
    constexpr auto required_size = fixed_encoded_size_v<T>;
    if (output.size() < required_size) {
        return std::unexpected(error{error_code::buffer_too_small, output.size()});
    }

    writer cursor{output};
    const auto result = detail::encode_value(value, cursor);
    if (!result) {
        return std::unexpected(result.error());
    }
    return cursor.offset();
}

template <class T>
    requires std::default_initializable<T>
[[nodiscard]] constexpr std::expected<T, error> decode(std::span<const std::byte> input) noexcept
{
    reader cursor{input};
    T value{};

    const auto result = detail::decode_value(value, cursor);
    if (!result) {
        return std::unexpected(result.error());
    }
    if (cursor.remaining() != 0) {
        return std::unexpected(error{error_code::trailing_bytes, cursor.offset()});
    }
    return value;
}

}  // namespace bite
