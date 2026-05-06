add_library(pob_polyhedral STATIC polyhedral_book.cpp)
target_include_directories(pob_polyhedral PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(pob_polyhedral PUBLIC highs::highs)
