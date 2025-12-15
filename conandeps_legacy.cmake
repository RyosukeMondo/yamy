message(STATUS "Conan: Using CMakeDeps conandeps_legacy.cmake aggregator via include()")
message(STATUS "Conan: It is recommended to use explicit find_package() per dependency instead")

find_package(quill)
find_package(Microsoft.GSL)
find_package(rapidcheck)
find_package(Catch2)
find_package(fmt)

set(CONANDEPS_LEGACY  quill::quill  Microsoft.GSL::GSL  rapidcheck::rapidcheck  Catch2::Catch2WithMain  fmt::fmt )