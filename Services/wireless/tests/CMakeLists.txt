# ==================================================================
# Хостові юніт-тести для Services/wireless/ (ТЗ §1.4 / §25).
#
# Це НЕ add_subdirectory() головного проєкту: корінь H750VB/CMakeLists.txt
# зафіксований на arm-none-eabi-gcc через toolchain-файл пресетів, а ці
# тести мусять збиратись нативним компілятором хоста. Тому це окремий,
# самостійний CMake-проєкт:
#
#   cmake -S Services/wireless/tests -B build/wireless-tests
#   cmake --build build/wireless-tests
#   ctest --test-dir build/wireless-tests
#
# rp_proto.c/rp_msg.c/network/**/*.c/port/rp_port_host.c не мають
# платформних залежностей (весь сенс rp_port.h) — саме тому це взагалі
# можливо без жодних правок у них під хост.
# ==================================================================
cmake_minimum_required(VERSION 3.22)
project(rp_wireless_tests C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# швидший gateway-keepalive для тесту прострочення вузла, ніж 120s-дефолт
# прошивки — форсимо CACHE-значення ДО include(), щоб rp_configure_network()
# сама підставила його через RP_IOT_KEEPALIVE_MS=${RP_IOT_KEEPALIVE_MS},
# а не задавати той самий -D двічі з різними значеннями (macro-redefined).
set(RP_IOT_KEEPALIVE_MS 5000 CACHE STRING "" FORCE)

include(${CMAKE_CURRENT_SOURCE_DIR}/../rp_config.cmake)

set(RP_WIRELESS ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(rp_core STATIC
    ${RP_WIRELESS}/rp_proto.c
    ${RP_WIRELESS}/rp_msg.c
)
target_include_directories(rp_core PUBLIC
    ${RP_WIRELESS}
    ${RP_WIRELESS}/massges
)

enable_testing()

# --- rp_proto / rp_msg -------------------------------------------------
add_executable(test_rp_proto test_rp_proto.c)
target_link_libraries(test_rp_proto PRIVATE rp_core)
add_test(NAME rp_proto COMMAND test_rp_proto)

# --- common/* (shared: frameq, duty) --------------------------------------
add_executable(test_common test_common.c
    ${RP_WIRELESS}/common/rp_frameq.c
    ${RP_WIRELESS}/common/rp_duty.c
)
target_link_libraries(test_common PRIVATE rp_core)
target_include_directories(test_common PRIVATE ${RP_WIRELESS}/common)
add_test(NAME common COMMAND test_common)

# --- network/iot/common/* (IoT-specific: nodetab, mailbox) -------------------
add_executable(test_iot_common test_iot_common.c
    ${RP_WIRELESS}/network/iot/common/rp_nodetab.c
    ${RP_WIRELESS}/network/iot/common/rp_mailbox.c
)
target_link_libraries(test_iot_common PRIVATE rp_core)
target_include_directories(test_iot_common PRIVATE ${RP_WIRELESS}/network/iot/common)
add_test(NAME iot_common COMMAND test_iot_common)

# --- port/rp_port_host --------------------------------------------------
add_executable(test_port_host test_port_host.c ${RP_WIRELESS}/port/rp_port_host.c)
target_include_directories(test_port_host PRIVATE ${RP_WIRELESS}/port)
add_test(NAME port_host COMMAND test_port_host)

# --- network/iot/rp_iot_leaf ----------------------------------------------
add_executable(test_iot_leaf test_iot_leaf.c mock_hw_if.c)
target_link_libraries(test_iot_leaf PRIVATE rp_core)
rp_configure_network(test_iot_leaf IOT host LEAF)
add_test(NAME iot_leaf COMMAND test_iot_leaf)

# --- network/iot/rp_iot_gateway --------------------------------------------
add_executable(test_iot_gateway test_iot_gateway.c mock_hw_if.c)
target_link_libraries(test_iot_gateway PRIVATE rp_core)
rp_configure_network(test_iot_gateway IOT host GATEWAY)
add_test(NAME iot_gateway COMMAND test_iot_gateway)

# --- network/p2p/{rp_neighbor,rp_route,rp_forward} (no queue/hw_if needed) ---
add_executable(test_p2p_common test_p2p_common.c
    ${RP_WIRELESS}/network/p2p/rp_neighbor.c
    ${RP_WIRELESS}/network/p2p/rp_route.c
    ${RP_WIRELESS}/network/p2p/rp_forward.c
)
target_link_libraries(test_p2p_common PRIVATE rp_core)
target_include_directories(test_p2p_common PRIVATE ${RP_WIRELESS}/network/p2p)
add_test(NAME p2p_common COMMAND test_p2p_common)

# --- network/p2p/rp_p2p (integration: discovery, forwarding, TTL, dedup) ----
add_executable(test_p2p test_p2p.c mock_hw_if.c)
target_link_libraries(test_p2p PRIVATE rp_core)
rp_configure_network(test_p2p P2P host)
add_test(NAME p2p COMMAND test_p2p)
