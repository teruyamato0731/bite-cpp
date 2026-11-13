#include <gtest/gtest.h>

#include <array>
#include <bite/bite.hpp>
#include <cstddef>
#include <cstdint>

namespace {

template <class T>
void expect_integer_round_trip(
    T value, const std::array<std::byte, sizeof(T)>& expected) {
  static_assert(bite::fixed_encoded_size_v<T> == sizeof(T));

  std::array<std::byte, bite::fixed_encoded_size_v<T>> buffer{};

  const auto encoded = bite::encode(value, buffer);
  ASSERT_TRUE(encoded.has_value());
  EXPECT_EQ(*encoded, buffer.size());
  EXPECT_EQ(buffer, expected);

  const auto decoded = bite::decode<T>(buffer);
  ASSERT_TRUE(decoded.has_value());
  EXPECT_EQ(*decoded, value);
}

}  // namespace

TEST(IntegerTest, EncodesUnsignedIntegersAsLittleEndian) {
  expect_integer_round_trip<std::uint8_t>(0xABu, {std::byte{0xAB}});
  expect_integer_round_trip<std::uint16_t>(0x1234u,
                                           {std::byte{0x34}, std::byte{0x12}});
  expect_integer_round_trip<std::uint32_t>(
      0x12345678u,
      {std::byte{0x78}, std::byte{0x56}, std::byte{0x34}, std::byte{0x12}});
  expect_integer_round_trip<std::uint64_t>(
      0x0123456789ABCDEFull,
      {std::byte{0xEF}, std::byte{0xCD}, std::byte{0xAB}, std::byte{0x89},
       std::byte{0x67}, std::byte{0x45}, std::byte{0x23}, std::byte{0x01}});
}

TEST(IntegerTest, EncodesNegativeSignedIntegersAsTwosComplement) {
  expect_integer_round_trip<std::int8_t>(static_cast<std::int8_t>(-2),
                                         {std::byte{0xFE}});
  expect_integer_round_trip<std::int16_t>(static_cast<std::int16_t>(-2),
                                          {std::byte{0xFE}, std::byte{0xFF}});
  expect_integer_round_trip<std::int32_t>(
      static_cast<std::int32_t>(-2),
      {std::byte{0xFE}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}});
  expect_integer_round_trip<std::int64_t>(
      static_cast<std::int64_t>(-2),
      {std::byte{0xFE}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
       std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}});
}
