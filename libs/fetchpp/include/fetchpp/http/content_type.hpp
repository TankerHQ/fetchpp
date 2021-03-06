#pragma once

#include <array>
#include <string>
#include <string_view>

namespace fetchpp::http
{
class content_type : private std::array<std::string, 3>
{
  using base_t = std::array<std::string, 3>;

public:
  content_type() = default;

  explicit content_type(std::string_view type,
                        std::string_view charset = "",
                        std::string_view boundary = "");

private:
  explicit content_type(std::string&& type,
                        std::string&& charset,
                        std::string&& boundary);

public:
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

std::string to_string(content_type const& ct);
}
