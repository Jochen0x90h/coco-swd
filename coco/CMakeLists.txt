set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
add_library(${PROJECT_NAME})
target_sources(${PROJECT_NAME}
    PUBLIC FILE_SET headers TYPE HEADERS FILES
        SwdDevice.hpp
    PRIVATE
        SwdDevice.cpp
)

if(${PLATFORM} STREQUAL "native" OR ${PLATFORM} STREQUAL "emu")
    # native platform (Windows, MacOS, Linux)
    #target_sources(${PROJECT_NAME}
    #    PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/native FILES
    #        native/coco/platform/dummy.hpp
    #    PRIVATE
    #        native/coco/platform/dummy.cpp
    #)
elseif(${PLATFORM} MATCHES "^nrf52")

elseif(${PLATFORM} MATCHES "^stm32")
    target_sources(${PROJECT_NAME}
        PUBLIC FILE_SET platform_headers TYPE HEADERS BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/stm32 FILES
            stm32/coco/platform/SwdDevice_SPI.hpp
        PRIVATE
            stm32/coco/platform/SwdDevice_SPI.cpp
    )
endif()

target_link_libraries(${PROJECT_NAME}
    coco::coco
    coco-loop::coco-loop
    coco-device::coco-device
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ..
)

# install the library
install(TARGETS ${PROJECT_NAME}
    FILE_SET headers DESTINATION include/coco
    FILE_SET platform_headers DESTINATION include
)
