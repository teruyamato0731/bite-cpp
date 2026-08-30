#pragma once

#include <cstddef>
#include <expected>
#include <span>

#include <bite/error.hpp>

namespace bite {

class writer {
public:
    constexpr explicit writer(std::span<std::byte> output) noexcept : output_(output) {}

    [[nodiscard]] constexpr std::size_t offset() const noexcept { return offset_; }
    [[nodiscard]] constexpr std::size_t remaining() const noexcept { return output_.size() - offset_; }

    [[nodiscard]] constexpr std::expected<void, error> write(std::span<const std::byte> bytes) noexcept
    {
        if (bytes.size() > remaining()) {
            return std::unexpected(error{error_code::buffer_too_small, output_.size()});
        }

        for (const auto byte : bytes) {
            output_[offset_++] = byte;
        }
        return {};
    }

private:
    std::span<std::byte> output_;
    std::size_t offset_{};
};

}  // namespace bite
