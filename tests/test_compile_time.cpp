#include <array>
#include <bite/bite.hpp>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <utility>

#include "test_types.hpp"

using bite_test::Padded;
using bite_test::Position;
using bite_test::RobotState;

static_assert(std::same_as<decltype(std::declval<RobotState&>().bite_fields()),
                           std::tuple<std::uint32_t&, Position&,
                                      std::array<std::int16_t, 4>&>>);
static_assert(
    std::same_as<decltype(std::declval<const RobotState&>().bite_fields()),
                 std::tuple<const std::uint32_t&, const Position&,
                            const std::array<std::int16_t, 4>&>>);

static_assert(bite::fixed_encoded_size_v<std::uint8_t> == 1);
static_assert(bite::fixed_encoded_size_v<std::int64_t> == 8);
static_assert(bite::fixed_encoded_size_v<bool> == 1);
static_assert(bite::fixed_encoded_size_v<float> == 4);
static_assert(bite::fixed_encoded_size_v<double> == 8);
static_assert(bite::fixed_encoded_size_v<bite_test::Mode> == 2);
static_assert(bite::fixed_encoded_size_v<std::array<std::uint16_t, 3>> == 6);
static_assert(
    bite::fixed_encoded_size_v<std::tuple<std::uint8_t, std::uint32_t>> == 5);
static_assert(bite::fixed_encoded_size_v<Padded> == 5);
static_assert(bite::fixed_encoded_size_v<RobotState> == 20);
static_assert(sizeof(Padded) >= bite::fixed_encoded_size_v<Padded>);

constexpr bool constexpr_round_trip() {
  const std::uint16_t value = 0x1234;
  std::array<std::byte, bite::fixed_encoded_size_v<decltype(value)>> buffer{};

  const auto encoded = bite::encode(value, buffer);
  const auto decoded = bite::decode<std::uint16_t>(buffer);

  return encoded.has_value() && *encoded == 2 && decoded.has_value() &&
         *decoded == value && buffer[0] == std::byte{0x34} &&
         buffer[1] == std::byte{0x12};
}

static_assert(constexpr_round_trip());
