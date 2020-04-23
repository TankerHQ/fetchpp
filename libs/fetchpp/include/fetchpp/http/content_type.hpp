#pragma once

#include <array>
#include <string>
#include <string_view>

#include <iosfwd>

namespace fetchpp
{
class content_type : private std::array<std::string, 3>
{
  using base_t = std::array<std::string, 3>;

public:
  content_type() = default;

  explicit content_type(std::string type,
                        std::string charset = "",
                        std::string boundary = "");

  static content_type parse(std::string_view sv);

  std::string const& type() const;
  void type(std::string_view);
  std::string const& charset() const;
  void charset(std::string_view);
  std::string const& boundary() const;
  void boundary(std::string_view);
};

bool operator==(content_type const& lhs, content_type const& rhs);
bool operator!=(content_type const& lhs, content_type const& rhs);

std::ostream& operator<<(std::ostream& os, content_type const& ct);
}
