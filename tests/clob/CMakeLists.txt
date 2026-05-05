add_executable(clob_tests smoke_test.cpp order_book_test.cpp)
target_link_libraries(clob_tests PRIVATE Catch2::Catch2WithMain pob_clob)

# Register each Catch2 TEST_CASE as its own ctest entry so `ctest -R <pattern>`
# can filter at the test-case level rather than seeing the binary as one test.
include(Catch)
catch_discover_tests(clob_tests)