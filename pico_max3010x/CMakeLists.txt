if (NOT TARGET pico_max3010x)
    add_library(pico_max3010x INTERFACE)

    target_sources(pico_max3010x INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/max3010x.cpp
        # ${CMAKE_CURRENT_LIST_DIR}/i2c.cpp
    )

    target_link_libraries(pico_max3010x INTERFACE pico_stdlib hardware_i2c)
    target_include_directories(pico_max3010x INTERFACE ${CMAKE_CURRENT_LIST_DIR}/../)
endif()
