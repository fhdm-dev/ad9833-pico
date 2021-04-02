add_library(fhdm-ad9833-pico INTERFACE)

target_sources(fhdm-ad9833-pico INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/fhdm-ad9833.c
  ${CMAKE_CURRENT_LIST_DIR}/include/fhdm-ad9833.h
  ${CMAKE_CURRENT_LIST_DIR}/fhdm-ad9833-pico.c
  ${CMAKE_CURRENT_LIST_DIR}/include/fhdm-ad9833-pico.h
)

target_include_directories(fhdm-ad9833-pico INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(fhdm-ad9833-pico INTERFACE hardware_spi hardware_gpio)
