# bool options
option(CHCORE_CONFIG_SMP "Use SMP" ON)
option(CHCORE_KERNEL_TEST "Enable Kernel Test" OFF)

# string options
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release." FORCE)
endif (NOT CMAKE_BUILD_TYPE)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release)

if (NOT CHCORE_PLAT)
set(CHCORE_PLAT "raspi3" CACHE STRING "Choose the platform, options are: raspi3 hikey970 intel.")
endif (NOT CHCORE_PLAT)
set_property(CACHE CHCORE_PLAT PROPERTY STRINGS raspi3 hikey970 intel)

if (NOT CHCORE_ARCH)
	set(CHCORE_ARCH "aarch64" CACHE STRING "Choose the arch, only aarch64 is supported now.")
endif (NOT CHCORE_ARCH)
set_property(CACHE CHCORE_ARCH PROPERTY STRINGS aarch64 x86_64)

mark_as_advanced(CMAKE_INSTALL_PREFIX)

# analyze options
if (FBINFER)
    add_definitions("-DFBINFER")
endif(FBINFER)

if (CHCORE_CONFIG_SMP)
    add_definitions("-DCHCORE_CONFIG_SMP")
endif(CHCORE_CONFIG_SMP)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions("-DLOG_LEVEL=2")
else ()
    add_definitions("-DLOG_LEVEL=1")
endif ()

if (CHCORE_KERNEL_TEST)
    add_definitions("-DKERNEL_TEST=1")
endif(CHCORE_KERNEL_TEST)

if (CHCORE_ARCH STREQUAL "x86_64")
    unset(CHCORE_PLAT)
    # default: intel (e.g., SGX feature) ; future: amd (e.g. SME feature)
    set(CHCORE_PLAT "intel")
endif ()
