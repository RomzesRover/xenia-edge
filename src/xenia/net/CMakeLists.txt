add_library(xenia-net STATIC)
xe_platform_sources(xenia-net ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(xenia-net PUBLIC mbedtls xenia-base)
xe_target_defaults(xenia-net)

if(XENIA_BUILD_TESTS)
  add_subdirectory(testing)
endif()
