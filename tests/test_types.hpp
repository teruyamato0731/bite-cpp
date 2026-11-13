#pragma once

#include <array>
#include <cstdint>
#include <tuple>

namespace bite_test {

enum class Mode : std::uint16_t {
  active = 0x1234,
};

struct Position {
  float x{};
  float y{};

  constexpr auto bite_fields(this auto& self) noexcept {
    return std::tie(self.x, self.y);
  }

  friend constexpr bool operator==(const Position&, const Position&) = default;
};

struct Padded {
  std::uint8_t a{};
  std::uint32_t b{};

  constexpr auto bite_fields(this auto& self) noexcept {
    return std::tie(self.a, self.b);
  }

  friend constexpr bool operator==(const Padded&, const Padded&) = default;
};

struct RobotState {
  std::uint32_t id{};
  Position position{};
  std::array<std::int16_t, 4> sensors{};

  constexpr auto bite_fields(this auto& self) noexcept {
    return std::tie(self.id, self.position, self.sensors);
  }

  friend constexpr bool operator==(const RobotState&,
                                   const RobotState&) = default;
};

struct BoolPacket {
  std::uint16_t prefix{};
  bool enabled{};

  constexpr auto bite_fields(this auto& self) noexcept {
    return std::tie(self.prefix, self.enabled);
  }
};

}  // namespace bite_test
