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
    const MotorCommand command{.velocity = 1.0F, .torque = 0.5F, .mode = 2};

    std::array<std::byte, bite::fixed_encoded_size_v<MotorCommand>> buffer{};
    const auto encoded = bite::encode(command, buffer);
    if (!encoded) {
        return 1;
    }

    const auto decoded = bite::decode<MotorCommand>(buffer);
    if (!decoded) {
        return 1;
    }

    return decoded->velocity == command.velocity && decoded->torque == command.torque &&
                   decoded->mode == command.mode
               ? 0
               : 1;
}
