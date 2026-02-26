zephyr_include_directories(include)

if(CONFIG_ZMK_RUNTIME_HOLD_TAP)
    target_sources(app PRIVATE src/runtime_hold_tap.c)

    if(CONFIG_ZMK_RUNTIME_HOLD_TAP_STUDIO_RPC)
        target_sources(app PRIVATE src/studio/custom_handler.c)

        list(APPEND CMAKE_MODULE_PATH ${ZEPHYR_BASE}/modules/nanopb)
        include(nanopb)
        set(NANOPB_GENERATE_CPP_APPEND_PATH TRUE)
        set(NANOPB_GENERATE_CPP_STANDALONE OFF)

        zephyr_library()
        file(GLOB_RECURSE PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)

        nanopb_generate_cpp(proto_srcs proto_hdrs RELPATH ${CMAKE_CURRENT_SOURCE_DIR} ${PROTO_FILES})
        target_include_directories(${ZEPHYR_CURRENT_LIBRARY} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
        target_sources(${ZEPHYR_CURRENT_LIBRARY} PRIVATE ${proto_srcs} ${proto_hdrs})

        target_include_directories(app PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/proto)
        if(DEFINED ZEPHYR_NANOPB_MODULE_DIR)
            target_include_directories(app PUBLIC ${ZEPHYR_NANOPB_MODULE_DIR})
        elseif(DEFINED NANOPB_DIR)
            target_include_directories(app PUBLIC ${NANOPB_DIR})
        endif()
        add_dependencies(app ${ZEPHYR_CURRENT_LIBRARY})
    endif()
endif()
