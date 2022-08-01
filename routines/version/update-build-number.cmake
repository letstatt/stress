if (NOT EXISTS "${BUILD_NUMBER_FILE}")
    set(build_number 1)
else()
    file(READ "${BUILD_NUMBER_FILE}" build_number)
    string(STRIP "${build_number}" build_number)
    math(EXPR build_number "${build_number} + 1")
endif()

file(WRITE "${BUILD_NUMBER_FILE}" "${build_number}\n")
