#pragma once
#include <cstdint>
#include <cassert>

class CBitFlags8
{
public:
    using storage_type = std::uint8_t;

    constexpr CBitFlags8() = default;
    constexpr explicit CBitFlags8(storage_type v) : bits_(v) {}

    constexpr bool operator[](std::size_t i) const noexcept
    {
        assert(i < 8);
        return (bits_ >> i) & 1u;
    }

    constexpr void set(std::size_t i, bool value = true) noexcept
    {
        assert(i < 8);
        if (value)
        {
            bits_ |= (storage_type(1) << i);
        }
        else
        {
            bits_ &= ~(storage_type(1) << i);
        }
    }

    constexpr void reset(std::size_t i) noexcept
    {
        set(i, false);
    }

    constexpr void toggle(std::size_t i) noexcept
    {
        assert(i < 8);
        bits_ ^= (storage_type(1) << i);
    }

    constexpr void clear() noexcept { bits_ = 0; }
    constexpr storage_type raw() const noexcept { return bits_; }
private:
    storage_type bits_ = 0;
};

static_assert(sizeof(CBitFlags8) == 1);