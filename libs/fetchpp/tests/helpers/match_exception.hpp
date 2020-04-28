#include <catch2/catch.hpp>

#include <fmt/format.h>

namespace test::helpers
{
class MatchExceptionErrorCode
  : public Catch::MatcherBase<boost::system::system_error>
{
  boost::system::error_code ref_;

public:
  MatchExceptionErrorCode(boost::system::error_code ec) : ref_(ec)
  {
  }

  bool match(boost::system::system_error const& value) const override
  {
    return value.code() == ref_;
  }

  std::string describe() const override
  {
    return fmt::format("is equivalent to the error_code ({}): '{}' ",
                       ref_.value(),
                       ref_.message());
  }
};

inline auto HasErrorCode(boost::system::error_code ec)
{
  return MatchExceptionErrorCode{ec};
}

}

CATCH_TRANSLATE_EXCEPTION(boost::system::system_error& ex)
{
  return fmt::format("({}): '{}'", ex.code(), ex.what());
}
