#pragma once

#ifndef NDEBUG
#if __has_include(<source_location>)
#include <source_location>
#define HSH_SOURCE_LOCATION_REP std::source_location
#elif __has_include(<experimental/source_location>)
#include <experimental/source_location>
#define HSH_SOURCE_LOCATION_REP std::experimental::source_location
#endif
#endif
#ifdef HSH_SOURCE_LOCATION_REP
#include <sstream>
namespace hsh {
class SourceLocation : public HSH_SOURCE_LOCATION_REP {
  const char *m_field = nullptr;
  std::uint32_t m_fieldIdx = UINT32_MAX;

public:
  SourceLocation(const HSH_SOURCE_LOCATION_REP &location,
                 const char *field = nullptr,
                 std::uint32_t fieldIdx = UINT32_MAX) noexcept
      : HSH_SOURCE_LOCATION_REP(location), m_field(field),
        m_fieldIdx(fieldIdx) {}
  SourceLocation with_field(const char *f, std::uint32_t idx = UINT32_MAX) const
      noexcept {
    return SourceLocation(*this, f, idx);
  }
  bool has_field() const noexcept { return m_field != nullptr; }
  const char *field() const noexcept { return m_field; }
  bool has_field_idx() const noexcept { return m_fieldIdx != UINT32_MAX; }
  std::uint32_t field_idx() const noexcept { return m_fieldIdx; }

  std::string to_string() const noexcept {
    std::ostringstream ss;
    ss << file_name() << ':' << line() << ' ' << function_name();
    if (has_field()) {
      ss << " (" << field() << ')';
      if (has_field_idx())
        ss << '[' << field_idx() << ']';
    }
    return ss.str();
  }
};
} // namespace hsh
#undef HSH_SOURCE_LOCATION_REP
#define HSH_SOURCE_LOCATION_ENABLED 1
#else
namespace hsh {
class SourceLocation {
public:
  static SourceLocation current() noexcept { return {}; }
};
} // namespace hsh
#define HSH_SOURCE_LOCATION_ENABLED 0
#endif
