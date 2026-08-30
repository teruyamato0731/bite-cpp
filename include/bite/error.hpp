#pragma once

#include <cstddef>

namespace bite {

enum class error_code {
    buffer_too_small,
    unexpected_end,
    trailing_bytes,
    invalid_bool,
};

struct error {
    error_code code;
    std::size_t offset;

    friend constexpr bool operator==(const error&, const error&) = default;
};

}  // namespace bite
