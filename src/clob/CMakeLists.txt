add_library(pob_clob STATIC order_book.cpp)
target_include_directories(pob_clob PUBLIC ${CMAKE_SOURCE_DIR}/include)
