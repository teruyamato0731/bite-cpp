#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <tuple>
#include <type_traits>
#include <utility>

#include <bite/bite.hpp>

namespace {

int failures = 0;

#define CHECK(...)                                                                                 \
    do {                                                                                           \
        if (!(__VA_ARGS__)) {                                                                      \
            std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #__VA_ARGS__); \
            ++failures;                                                                            \
        }                                                                                          \
    } while (false)

template <std::size_t N>
void check_bytes(
    const std::array<std::byte, N>& actual, const std::array<std::byte, N>& expected)
{
    CHECK(actual == expected);
}

template <class T>
void check_integer_round_trip(T value, const std::array<std::byte, sizeof(T)>& expected)
{
    std::array<std::byte, bite::fixed_encoded_size_v<T>> buffer{};
    const auto encoded = bite::encode(value, buffer);
    CHECK(encoded.has_value());
    if (encoded) {
        CHECK(*encoded == buffer.size());
    }
    check_bytes(buffer, expected);

    const auto decoded = bite::decode<T>(buffer);
    CHECK(decoded.has_value());
    if (decoded) {
        CHECK(*decoded == value);
    }
}

enum class Mode : std::uint16_t {
    active = 0x1234,
};

struct Position {
    float x{};
    float y{};

    constexpr auto bite_fields(this auto& self) noexcept
    {
        return std::tie(self.x, self.y);
    }

    friend constexpr bool operator==(const Position&, const Position&) = default;
};

struct Padded {
    std::uint8_t a{};
    std::uint32_t b{};

    constexpr auto bite_fields(this auto& self) noexcept
    {
        return std::tie(self.a, self.b);
    }

    friend constexpr bool operator==(const Padded&, const Padded&) = default;
};

struct RobotState {
    std::uint32_t id{};
    Position position{};
    std::array<std::int16_t, 4> sensors{};

    constexpr auto bite_fields(this auto& self) noexcept
    {
        return std::tie(self.id, self.position, self.sensors);
    }

    friend constexpr bool operator==(const RobotState&, const RobotState&) = default;
};

struct BoolPacket {
    std::uint16_t prefix{};
    bool enabled{};

    constexpr auto bite_fields(this auto& self) noexcept
    {
        return std::tie(self.prefix, self.enabled);
    }
};

static_assert(std::same_as<
              decltype(std::declval<RobotState&>().bite_fields()),
              std::tuple<std::uint32_t&, Position&, std::array<std::int16_t, 4>&>>);
static_assert(std::same_as<
              decltype(std::declval<const RobotState&>().bite_fields()),
              std::tuple<const std::uint32_t&, const Position&, const std::array<std::int16_t, 4>&>>);

static_assert(bite::fixed_encoded_size_v<std::uint8_t> == 1);
static_assert(bite::fixed_encoded_size_v<std::int64_t> == 8);
static_assert(bite::fixed_encoded_size_v<bool> == 1);
static_assert(bite::fixed_encoded_size_v<float> == 4);
static_assert(bite::fixed_encoded_size_v<double> == 8);
static_assert(bite::fixed_encoded_size_v<Mode> == 2);
static_assert(bite::fixed_encoded_size_v<std::array<std::uint16_t, 3>> == 6);
static_assert(bite::fixed_encoded_size_v<std::tuple<std::uint8_t, std::uint32_t>> == 5);
static_assert(bite::fixed_encoded_size_v<Padded> == 5);
static_assert(bite::fixed_encoded_size_v<RobotState> == 20);
static_assert(sizeof(Padded) >= bite::fixed_encoded_size_v<Padded>);

constexpr bool constexpr_round_trip()
{
    const std::uint16_t value = 0x1234;
    std::array<std::byte, bite::fixed_encoded_size_v<decltype(value)>> buffer{};
    const auto encoded = bite::encode(value, buffer);
    const auto decoded = bite::decode<std::uint16_t>(buffer);
    return encoded.has_value() && *encoded == 2 && decoded.has_value() && *decoded == value &&
           buffer[0] == std::byte{0x34} && buffer[1] == std::byte{0x12};
}

static_assert(constexpr_round_trip());

void test_integers()
{
    check_integer_round_trip<std::uint8_t>(0xABu, {std::byte{0xAB}});
    check_integer_round_trip<std::int8_t>(static_cast<std::int8_t>(-2), {std::byte{0xFE}});
    check_integer_round_trip<std::uint16_t>(0x1234u, {std::byte{0x34}, std::byte{0x12}});
    check_integer_round_trip<std::int16_t>(static_cast<std::int16_t>(-2), {std::byte{0xFE}, std::byte{0xFF}});
    check_integer_round_trip<std::uint32_t>(
        0x12345678u, {std::byte{0x78}, std::byte{0x56}, std::byte{0x34}, std::byte{0x12}});
    check_integer_round_trip<std::int32_t>(
        static_cast<std::int32_t>(-2),
        {std::byte{0xFE}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}});
    check_integer_round_trip<std::uint64_t>(
        0x0123456789ABCDEFull,
        {std::byte{0xEF}, std::byte{0xCD}, std::byte{0xAB}, std::byte{0x89},
         std::byte{0x67}, std::byte{0x45}, std::byte{0x23}, std::byte{0x01}});
    check_integer_round_trip<std::int64_t>(
        static_cast<std::int64_t>(-2),
        {std::byte{0xFE}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF},
         std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}, std::byte{0xFF}});
}

void test_bool()
{
    for (const auto value : {false, true}) {
        std::array<std::byte, 1> buffer{};
        const auto encoded = bite::encode(value, buffer);
        CHECK(encoded.has_value());
        CHECK(buffer[0] == (value ? std::byte{0x01} : std::byte{0x00}));

        const auto decoded = bite::decode<bool>(buffer);
        CHECK(decoded.has_value());
        if (decoded) {
            CHECK(*decoded == value);
        }
    }

    const std::array invalid{std::byte{0x02}};
    const auto decoded = bite::decode<bool>(invalid);
    CHECK(!decoded.has_value());
    if (!decoded) {
        CHECK(decoded.error() == bite::error{bite::error_code::invalid_bool, 0});
    }
}

void test_floating_point()
{
    {
        const float value = 1.0F;
        std::array<std::byte, 4> buffer{};
        CHECK(bite::encode(value, buffer).has_value());
        check_bytes(buffer, std::array{std::byte{0x00}, std::byte{0x00}, std::byte{0x80}, std::byte{0x3F}});
        const auto decoded = bite::decode<float>(buffer);
        CHECK(decoded.has_value());
        if (decoded) {
            CHECK(std::bit_cast<std::uint32_t>(*decoded) == std::bit_cast<std::uint32_t>(value));
        }
    }

    {
        const double value = 1.0;
        std::array<std::byte, 8> buffer{};
        CHECK(bite::encode(value, buffer).has_value());
        check_bytes(
            buffer,
            std::array{std::byte{0x00}, std::byte{0x00}, std::byte{0x00}, std::byte{0x00},
                       std::byte{0x00}, std::byte{0x00}, std::byte{0xF0}, std::byte{0x3F}});
        const auto decoded = bite::decode<double>(buffer);
        CHECK(decoded.has_value());
        if (decoded) {
            CHECK(std::bit_cast<std::uint64_t>(*decoded) == std::bit_cast<std::uint64_t>(value));
        }
    }
}

void test_enum()
{
    std::array<std::byte, 2> buffer{};
    CHECK(bite::encode(Mode::active, buffer).has_value());
    check_bytes(buffer, std::array{std::byte{0x34}, std::byte{0x12}});

    const auto decoded = bite::decode<Mode>(buffer);
    CHECK(decoded.has_value());
    if (decoded) {
        CHECK(*decoded == Mode::active);
    }
}

void test_array()
{
    const std::array<std::uint16_t, 3> value{0x0102, 0x0304, 0x0506};
    std::array<std::byte, bite::fixed_encoded_size_v<decltype(value)>> buffer{};
    CHECK(bite::encode(value, buffer).has_value());
    check_bytes(
        buffer,
        std::array{std::byte{0x02}, std::byte{0x01}, std::byte{0x04},
                   std::byte{0x03}, std::byte{0x06}, std::byte{0x05}});

    const auto decoded = bite::decode<std::array<std::uint16_t, 3>>(buffer);
    CHECK(decoded.has_value());
    if (decoded) {
        CHECK(*decoded == value);
    }

    const std::array<Position, 2> nested{{Position{1.0F, 2.0F}, Position{3.0F, 4.0F}}};
    std::array<std::byte, bite::fixed_encoded_size_v<decltype(nested)>> nested_buffer{};
    CHECK(bite::encode(nested, nested_buffer).has_value());
    const auto nested_decoded = bite::decode<std::array<Position, 2>>(nested_buffer);
    CHECK(nested_decoded.has_value());
    if (nested_decoded) {
        CHECK(*nested_decoded == nested);
    }
}

void test_tuple()
{
    const auto value = std::tuple{std::uint8_t{0x11}, std::uint16_t{0x2233}, true};
    std::array<std::byte, bite::fixed_encoded_size_v<decltype(value)>> buffer{};
    CHECK(bite::encode(value, buffer).has_value());
    check_bytes(buffer, std::array{std::byte{0x11}, std::byte{0x33}, std::byte{0x22}, std::byte{0x01}});

    const auto decoded = bite::decode<std::remove_cv_t<decltype(value)>>(buffer);
    CHECK(decoded.has_value());
    if (decoded) {
        CHECK(*decoded == value);
    }
}

void test_structs()
{
    const Padded padded{.a = 0x12, .b = 0x3456789A};
    std::array<std::byte, bite::fixed_encoded_size_v<Padded>> padded_buffer{};
    CHECK(bite::encode(padded, padded_buffer).has_value());
    check_bytes(
        padded_buffer,
        std::array{std::byte{0x12}, std::byte{0x9A}, std::byte{0x78}, std::byte{0x56}, std::byte{0x34}});
    const auto padded_decoded = bite::decode<Padded>(padded_buffer);
    CHECK(padded_decoded.has_value());
    if (padded_decoded) {
        CHECK(*padded_decoded == padded);
    }

    const RobotState state{
        .id = 0x12345678,
        .position = {.x = 1.0F, .y = -2.0F},
        .sensors = {1, -2, 300, -400},
    };
    std::array<std::byte, bite::fixed_encoded_size_v<RobotState>> buffer{};
    const auto encoded = bite::encode(state, buffer);
    CHECK(encoded.has_value());
    if (encoded) {
        CHECK(*encoded == buffer.size());
    }

    const auto decoded = bite::decode<RobotState>(buffer);
    CHECK(decoded.has_value());
    if (decoded) {
        CHECK(*decoded == state);
    }
}

void test_errors()
{
    {
        std::array<std::byte, 4> storage{};
        storage.fill(std::byte{0xAA});
        const auto result = bite::encode(std::uint32_t{0x12345678}, std::span{storage}.first<3>());
        CHECK(!result.has_value());
        if (!result) {
            CHECK(result.error() == bite::error{bite::error_code::buffer_too_small, 3});
        }
        CHECK(storage == std::array{std::byte{0xAA}, std::byte{0xAA}, std::byte{0xAA}, std::byte{0xAA}});
    }

    {
        const std::array input{std::byte{0x34}, std::byte{0x12}};
        const auto result = bite::decode<std::uint32_t>(input);
        CHECK(!result.has_value());
        if (!result) {
            CHECK(result.error() == bite::error{bite::error_code::unexpected_end, 2});
        }
    }

    {
        const std::array input{std::byte{0x34}, std::byte{0x12}, std::byte{0xFF}};
        const auto result = bite::decode<std::uint16_t>(input);
        CHECK(!result.has_value());
        if (!result) {
            CHECK(result.error() == bite::error{bite::error_code::trailing_bytes, 2});
        }
    }

    {
        const std::array input{std::byte{0x34}, std::byte{0x12}, std::byte{0x02}};
        const auto result = bite::decode<BoolPacket>(input);
        CHECK(!result.has_value());
        if (!result) {
            CHECK(result.error() == bite::error{bite::error_code::invalid_bool, 2});
        }
    }
}

}  // namespace

int main()
{
    test_integers();
    test_bool();
    test_floating_point();
    test_enum();
    test_array();
    test_tuple();
    test_structs();
    test_errors();

    if (failures != 0) {
        std::fprintf(stderr, "%d test checks failed\n", failures);
        return 1;
    }
    return 0;
}
