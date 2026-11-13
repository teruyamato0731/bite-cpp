#include <gtest/gtest.h>

#include <array>
#include <bite/bite.hpp>
#include <cstddef>
#include <cstdint>
#include <span>

#include "test_types.hpp"

TEST(EncodingErrorTest, TooSmallBufferDoesNotModifyOutput) {
  std::array<std::byte, 4> storage{};
  storage.fill(std::byte{0xAA});

  const auto result =
      bite::encode(std::uint32_t{0x12345678}, std::span{storage}.first<3>());

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            (bite::error{bite::error_code::buffer_too_small, 3}));
  EXPECT_EQ(storage, (std::array{
                         std::byte{0xAA},
                         std::byte{0xAA},
                         std::byte{0xAA},
                         std::byte{0xAA},
                     }));
}

TEST(DecodingErrorTest, ReportsUnexpectedEndOffset) {
  const std::array input{std::byte{0x34}, std::byte{0x12}};

  const auto result = bite::decode<std::uint32_t>(input);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), (bite::error{bite::error_code::unexpected_end, 2}));
}

TEST(DecodingErrorTest, RejectsTrailingBytes) {
  const std::array input{
      std::byte{0x34},
      std::byte{0x12},
      std::byte{0xFF},
  };

  const auto result = bite::decode<std::uint16_t>(input);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), (bite::error{bite::error_code::trailing_bytes, 2}));
}

TEST(DecodingErrorTest, ReportsInvalidBoolOffsetInStruct) {
  const std::array input{
      std::byte{0x34},
      std::byte{0x12},
      std::byte{0x02},
  };

  const auto result = bite::decode<bite_test::BoolPacket>(input);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), (bite::error{bite::error_code::invalid_bool, 2}));
}

TEST(DecodingErrorTest, RejectsInvalidStandaloneBool) {
  const std::array input{std::byte{0x02}};

  const auto result = bite::decode<bool>(input);

  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), (bite::error{bite::error_code::invalid_bool, 0}));
}
