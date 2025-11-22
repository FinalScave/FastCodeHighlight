set(TEST_PRODUCT_NAME test)
add_executable(${TEST_PRODUCT_NAME}
        ${3DPARTY_DIR}/include/catch2/catch_amalgamated.cpp
        patch_text.cpp
        parse_rule.cpp
)

target_include_directories(${TEST_PRODUCT_NAME} PRIVATE
        ${3DPARTY_DIR}/include
        ${SRC_DIR}/include
)

target_link_libraries(${TEST_PRODUCT_NAME} PRIVATE
        fast-highlight
)

enable_testing()
add_test(NAME UnitTests COMMAND ${TEST_PRODUCT_NAME})