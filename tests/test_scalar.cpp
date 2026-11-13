#include <gtest/gtest.h>

#include <array>
#include <bit>
#include <bite/bite.hpp>
#include <cstddef>
#include <cstdint>

#include "test_types.hpp"

TEST(ScalarTest, BoolRoundTripsFalseAndTrue) {
  for (const bool value : std::array{false, true}) {
    std::array<std::byte, 1> buffer{};

    const auto encoded = bite::encode(value, buffer);
    ASSERT_TRUE(encoded.has_value());
    EXPECT_EQ(*encoded, 1u);
    EXPECT_EQ(buffer[0], value ? std::byte{0x01} : std::byte{0x00});

    const auto decoded = bite::decode<bool>(buffer);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(*decoded, value);
  }
}

TEST(ScalarTest, EncodesFloatAsIeee754Binary32LittleEndian) {
  constexpr float value = 1.0F;
  constexpr std::array expected{
      std::byte{0x00},
      std::byte{0x00},
      std::byte{0x80},
      std::byte{0x3F},
  };
  std::array<std::byte, 4> buffer{};

  const auto encoded = bite::encode(value, buffer);
  ASSERT_TRUE(encoded.has_value());
  EXPECT_EQ(buffer, expected);

  const auto decoded = bite::decode<float>(buffer);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(std::bit_cast<std::uint32_t>(*decoded),
            std::bit_cast<std::uint32_t>(value));
}

TEST(ScalarTest, EncodesDoubleAsIeee754Binary64LittleEndian) {
  constexpr double value = 1.0;
  constexpr std::array expected{
      std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
      std::byte{0x00}, std::byte{0x00}, std::byte{0xF0}, std::byte{0x3F},
  };
  std::array<std::byte, 8> buffer{};

  const auto encoded = bite::encode(value, buffer);
  ASSERT_TRUE(encoded.has_value());
  EXPECT_EQ(buffer, expected);

  const auto decoded = bite::decode<double>(buffer);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(std::bit_cast<std::uint64_t>(*decoded),
            std::bit_cast<std::uint64_t>(value));
}

TEST(ScalarTest, EncodesEnumUsingItsUnderlyingType) {
  std::array<std::byte, 2> buffer{};

  const auto encoded = bite::encode(bite_test::Mode::active, buffer);
  ASSERT_TRUE(encoded.has_value());
  EXPECT_EQ(buffer, (std::array{std::byte{0x34}, std::byte{0x12}}));

  const auto decoded = bite::decode<bite_test::Mode>(buffer);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(*decoded, bite_test::Mode::active);
}
