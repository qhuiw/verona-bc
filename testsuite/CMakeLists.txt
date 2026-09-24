include("${trieste_SOURCE_DIR}/cmake/testsuite.cmake")

# Use Trieste to test Verona bytecode tools (vbcc and vbci)
testsuite(vbc)

add_test(
	NAME vbc/virc/vbcc-compat
	COMMAND "${CMAKE_INSTALL_PREFIX}/vbcc/$<TARGET_FILE_NAME:vbcc>" --help)
