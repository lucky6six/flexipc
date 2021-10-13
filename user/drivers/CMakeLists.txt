cmake_minimum_required(VERSION 3.11)

add_executable(driver_hub.srv
    driver_hub.c
    devtree.c
    dtb.c
)

if (CHCORE_CONFIG_UFS)
    add_subdirectory(ufs)
endif()
