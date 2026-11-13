#include <gtest/gtest.h>

#include <array>
#include <bite/bite.hpp>
#include <cstddef>
#include <cstdint>
#include <tuple>
#include <type_traits>

#include "test_types.hpp"

TEST(CompositeTest, EncodesArrayElementsInOrder) {
  const std::array<std::uint16_t, 3> value{0x0102, 0x0304, 0x0506};
  constexpr std::array expected{
      std::byte{0x02}, std::byte{0x01}, std::byte{0x04},
      std::byte{0x03}, std::byte{0x06}, std::byte{0x05},
  };
  std::array<std::byte, bite::fixed_encoded_size_v<decltype(value)>> buffer{};

  const auto encoded = bite::encode(value, buffer);
  ASSERT_TRUE(encoded.has_value());
  EXPECT_EQ(buffer, expected);

  const auto decoded = bite::decode<std::array<std::uint16_t, 3>>(buffer);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(*decoded, value);
}

TEST(CompositeTest, EncodesArraysOfUserDefinedTypes) {
  const std::array<bite_test::Position, 2> value{
      bite_test::Position{1.0F, 2.0F},
      bite_test::Position{3.0F, 4.0F},
  };
  std::array<std::byte, bite::fixed_encoded_size_v<decltype(value)>> buffer{};

  const auto encoded = bite::encode(value, buffer);
  ASSERT_TRUE(encoded.has_value());

  const auto decoded = bite::decode<std::array<bite_test::Position, 2>>(buffer);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(*decoded, value);
}

TEST(CompositeTest, EncodesTupleElementsInOrder) {
  const auto value =
      std::tuple{std::uint8_t{0x11}, std::uint16_t{0x2233}, true};
  constexpr std::array expected{
      std::byte{0x11},
      std::byte{0x33},
      std::byte{0x22},
      std::byte{0x01},
  };
  std::array<std::byte, bite::fixed_encoded_size_v<decltype(value)>> buffer{};

  const auto encoded = bite::encode(value, buffer);
  ASSERT_TRUE(encoded.has_value());
  EXPECT_EQ(buffer, expected);

  const auto decoded = bite::decode<std::remove_cv_t<decltype(value)>>(buffer);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(*decoded, value);
}

TEST(CompositeTest, EncodesUserDefinedStructWithoutPadding) {
  const bite_test::Padded value{.a = 0x12, .b = 0x3456789A};
  constexpr std::array expected{
      std::byte{0x12}, std::byte{0x9A}, std::byte{0x78},
      std::byte{0x56}, std::byte{0x34},
  };
  std::array<std::byte, bite::fixed_encoded_size_v<bite_test::Padded>> buffer{};

  const auto encoded = bite::encode(value, buffer);
  ASSERT_TRUE(encoded.has_value());
  EXPECT_EQ(buffer, expected);

  const auto decoded = bite::decode<bite_test::Padded>(buffer);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(*decoded, value);
}

TEST(CompositeTest, RoundTripsNestedUserDefinedStruct) {
  const bite_test::RobotState value{
      .id = 0x12345678,
      .position = {.x = 1.0F, .y = -2.0F},
      .sensors = {1, -2, 300, -400},
  };
  std::array<std::byte, bite::fixed_encoded_size_v<bite_test::RobotState>>
      buffer{};

  const auto encoded = bite::encode(value, buffer);
  ASSERT_TRUE(encoded.has_value());
  EXPECT_EQ(*encoded, buffer.size());

  const auto decoded = bite::decode<bite_test::RobotState>(buffer);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(*decoded, value);
}
