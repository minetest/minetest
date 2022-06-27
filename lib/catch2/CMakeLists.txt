# catch2 is distributed as a standalone header.
#
# Downloaded from:
#
#      https://github.com/catchorg/Catch2/releases/download/v2.13.9/catch.hpp
#
# The following changes were made to always print in microseconds, fixed format:
#
# -    explicit Duration(double inNanoseconds, Unit units = Unit::Auto)
# +    explicit Duration(double inNanoseconds, Unit units = Unit::Microseconds)
#
# -        return os << duration.value() << ' ' << duration.unitsAsString();
# +        return os << std::fixed << duration.value() << ' ' << duration.unitsAsString();

add_library(catch2 INTERFACE)
target_include_directories(catch2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
