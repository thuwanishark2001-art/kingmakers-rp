#pragma once
#include <concepts>
#include <type_traits>
#include <cstdint>

//! Fixed point number (With implicit conversion to float)
template<std::integral T, int CompressNum, int CompressDen = 1, bool UseRoundingWhenConverting = false>
class FixedFloat {
public:
    static constexpr float CompressValue = static_cast<float>(CompressNum) / static_cast<float>(CompressDen);

    //! Construct as 0
    constexpr FixedFloat() = default;

    //! Construct from an uncompressed value
    constexpr FixedFloat(float v) { Set(v, UseRoundingWhenConverting); }

    //! Construct from a pre-compressed value
    template<std::integral Y>
    explicit constexpr FixedFloat(Y x) : value(x) {}

    //! Construct from another `FixedFloat` with the same CompressValue
    template<std::integral Y>
    constexpr FixedFloat(FixedFloat<Y, CompressNum, CompressDen> x) : value(x.value) {}

    //! Construct from another `FixedFloat` with a different CompressValue
    template<std::integral Y, int CN, int CD>
    requires (CN != CompressNum || CD != CompressDen)
    constexpr FixedFloat(FixedFloat<Y, CN, CD> x) : FixedFloat{(float)x} {}

    constexpr operator float() const { return static_cast<float>(value) / CompressValue; }

    //! Set the value (Use this if you want to set using rounding)
    constexpr void Set(float v, bool round = UseRoundingWhenConverting) {
        value = round ? static_cast<T>(v * CompressValue + 0.5f) : static_cast<T>(v * CompressValue);
    }

#define IMPLEMENT_FIXED_OPERATOR(_op) \
    template<typename T2, typename CommonT = std::common_type_t<T, T2>> \
    constexpr auto operator _op(FixedFloat<T2, CompressNum, CompressDen> o) { \
        return FixedFloat<CommonT, CompressNum, CompressDen>{static_cast<CommonT>(this->value) _op static_cast<CommonT>(o.value)}; \
    }

    IMPLEMENT_FIXED_OPERATOR(+);
    IMPLEMENT_FIXED_OPERATOR(-);
    IMPLEMENT_FIXED_OPERATOR(/);
    IMPLEMENT_FIXED_OPERATOR(*);

public:
    T value{};
};
