#ifndef BITE_READER_HPP
#define BITE_READER_HPP

#include <cstddef>
#include <expected>
#include <span>

#include <bite/error.hpp>

namespace bite {

class reader {
public:
    constexpr explicit reader(std::span<const std::byte> input) noexcept : input_(input) {}

    [[nodiscard]] constexpr std::size_t offset() const noexcept { return offset_; }
    [[nodiscard]] constexpr std::size_t remaining() const noexcept { return input_.size() - offset_; }

    [[nodiscard]] constexpr std::expected<std::span<const std::byte>, error> read(std::size_t count) noexcept
    {
        if (count > remaining()) {
            return std::unexpected(error{error_code::unexpected_end, input_.size()});
        }

        const auto bytes = input_.subspan(offset_, count);
        offset_ += count;
        return bytes;
    }

private:
    std::span<const std::byte> input_;
    std::size_t offset_{};
};

}  // namespace bite

#endif  // BITE_READER_HPP
