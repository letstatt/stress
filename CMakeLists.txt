cmake_minimum_required(VERSION 3.15)
project(stress)

set(CMAKE_CXX_STANDARD 20)

include_directories(include include/global)
set(sources)

# os-independent sources
LIST(APPEND sources
        "src/global/main.cpp"
        "src/global/stress.cpp"
        "src/global/terminal.cpp"
        "src/global/invoker.cpp"
        "src/global/logger.cpp"
        "src/global/core/run.cpp"
        "src/global/core/session.cpp"
        "src/global/parsing/args.cpp"
        "src/global/units/unit.cpp"
        "src/global/units/generator.cpp"
        "src/global/units/to_test.cpp"
        "src/global/units/prime.cpp"
        "src/global/units/verifier.cpp")

# os-dependent sources
if (WIN32)
    list(APPEND sources
            "src/win/core/error_info.cpp"
            "src/win/core/run.cpp"
            "src/win/terminal.cpp"
            "src/win/invoker.cpp")
else()
    list(APPEND sources
            "src/linux/core/error_info.cpp"
            "src/linux/core/run.cpp"
            "src/linux/core/maps.cpp"
            "src/linux/terminal.cpp"
            "src/linux/invoker.cpp"
            "src/linux/parsing/proc_parser.cpp")
endif()

# options
if (WIN32)
    if (MSVC)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    else()
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif()
else()
    set(CMAKE_CXX_LINK_FLAGS "${CMAKE_CXX_LINK_FLAGS} -latomic -pthread")
endif()

# build
add_executable(stress ${sources})

if (!MSVC)
    target_compile_options(stress PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
