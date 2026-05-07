add_executable(polyhedral_tests highs_smoke_test.cpp polyhedral_book_test.cpp predicates_test.cpp pricing_test.cpp)
target_link_libraries(polyhedral_tests PRIVATE Catch2::Catch2WithMain pob_polyhedral)

# Register each Catch2 TEST_CASE as its own ctest entry so `ctest -R <pattern>`
# can filter at the test-case level rather than seeing the binary as one test.
include(Catch)
catch_discover_tests(polyhedral_tests)