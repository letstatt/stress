file(READ "${BUILD_NUMBER_FILE}" build_number)
string(STRIP "${build_number}" build_number)

string(TIMESTAMP timestamp)

configure_file(${IN_FILE} ${CPP_FILE})
