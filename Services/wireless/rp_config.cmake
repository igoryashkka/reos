# ==================================================================
# rp_config.cmake — compile-time вибір wireless network model (ТЗ §5-6)
#
#   RP_NETWORK_MODEL = P2P | IOT
#   RP_IOT_ROLE       = LEAF | ROUTER | GATEWAY   (лише коли IOT)
#
# Дві окремі STRING cache-змінні з фіксованим переліком значень, а не
# булеві RP_NETWORK_P2P/RP_NETWORK_IOT чи RP_IOT_ROLE_LEAF/...: набір
# булевих дозволив би ввімкнути дві моделі чи дві ролі одночасно, а
# компіляція обох реалізацій з runtime-вибором заборонена (§22-23).
# Один STRING на вимір унеможливлює конфлікт структурно.
#
# Host-тести (Services/wireless/tests/) НЕ додають третє значення сюди
# (на відміну від старого RP_ROLE=host_test) — це окремий, самостійний
# CMake-проєкт з власним нативним компілятором, і кожен тестовий таргет
# викликає rp_configure_network() напряму зі своєю моделлю/роллю,
# незалежно від цієї cache-змінної (в одному прогоні тестів збираються
# одразу кілька моделей/ролей — RP_NETWORK_MODEL не може нести кілька
# значень одночасно).
# ==================================================================

set(RP_WIRELESS_DIR ${CMAKE_CURRENT_LIST_DIR})

set(RP_NETWORK_MODEL "IOT" CACHE STRING "Wireless network model: P2P | IOT")
set_property(CACHE RP_NETWORK_MODEL PROPERTY STRINGS P2P IOT)

set(RP_NETWORK_MODEL_VALID_VALUES P2P IOT)
if(NOT RP_NETWORK_MODEL IN_LIST RP_NETWORK_MODEL_VALID_VALUES)
    message(FATAL_ERROR
        "RP_NETWORK_MODEL='${RP_NETWORK_MODEL}' is not valid. Choose exactly one of: ${RP_NETWORK_MODEL_VALID_VALUES}")
endif()

set(RP_IOT_ROLE "LEAF" CACHE STRING "IoT role (used only when RP_NETWORK_MODEL=IOT): LEAF | ROUTER | GATEWAY")
set_property(CACHE RP_IOT_ROLE PROPERTY STRINGS LEAF ROUTER GATEWAY)

set(RP_IOT_ROLE_VALID_VALUES LEAF ROUTER GATEWAY)
if(RP_NETWORK_MODEL STREQUAL "IOT" AND NOT RP_IOT_ROLE IN_LIST RP_IOT_ROLE_VALID_VALUES)
    message(FATAL_ERROR
        "RP_IOT_ROLE='${RP_IOT_ROLE}' is not valid. Choose exactly one of: ${RP_IOT_ROLE_VALID_VALUES}")
endif()

# --- §24 Common ------------------------------------------------------
set(RP_FRAMEQ_DEPTH 8   CACHE STRING "common: default rp_frameq_t capacity")
set(RP_DUTY_LIMIT   100 CACHE STRING "common: duty-cycle soft limit percent (0-100)")

# --- §24 IoT -----------------------------------------------------------
set(RP_LEAF_QUEUE_DEPTH  16     CACHE STRING "IOT/LEAF: reading queue depth (drop-oldest)")
set(RP_GW_MAX_NODES      64     CACHE STRING "IOT/GATEWAY: nodetab capacity (full scale)")
set(RP_ROUTER_MAX_NODES  8      CACHE STRING "IOT/ROUTER: nodetab capacity (direct children only)")
set(RP_IOT_KEEPALIVE_MS  120000 CACHE STRING "IOT: gateway/router keepalive timeout")
set(RP_IOT_RX_WINDOW_MS  500    CACHE STRING "IOT: leaf/router rx window duration")

# --- §24 P2P ---------------------------------------------------------------
set(RP_P2P_MAX_NEIGHBORS      16    CACHE STRING "P2P: neighbor table capacity")
set(RP_P2P_MAX_ROUTES         16    CACHE STRING "P2P: route table capacity")
set(RP_P2P_ROUTE_TIMEOUT_MS   60000 CACHE STRING "P2P: route entry expiry")
set(RP_P2P_DIRECT_MIN_QUALITY 60    CACHE STRING "P2P: 0-100 min link quality to prefer direct send over multi-hop")
set(RP_P2P_MAX_TTL            8     CACHE STRING "P2P: max hop count before a frame is dropped")
set(RP_P2P_MAX_RETRIES        3     CACHE STRING "P2P: max retransmit attempts (reserved, not yet consumed)")

# rp_configure_network(<target> <model> <port> [<iot_role>])
#
# Додає до ${target} рівно одну мережеву реалізацію — network/p2p/rp_p2p.c
# або network/iot/rp_iot_<iot_role>.c — плюс спільний common/*.c, порт
# часу (freertos для прошивки, host — для tests/wireless/) і -D за
# моделлю/роллю. model/port/iot_role — явні аргументи, а не читання
# глобальних RP_NETWORK_MODEL/RP_IOT_ROLE: так один виклик обслуговує і
# прошивку, і кожен окремий host-тестовий таргет (§25).
function(rp_configure_network target model port)
    set(_valid_models P2P IOT)
    if(NOT model IN_LIST _valid_models)
        message(FATAL_ERROR "rp_configure_network(${target}): model='${model}' invalid, choose one of: ${_valid_models}")
    endif()
    if(NOT port STREQUAL "freertos" AND NOT port STREQUAL "host")
        message(FATAL_ERROR "rp_configure_network(${target}): port='${port}' invalid, choose 'freertos' or 'host'")
    endif()

    target_include_directories(${target} PRIVATE
        ${RP_WIRELESS_DIR}/network
        ${RP_WIRELESS_DIR}/common
        ${RP_WIRELESS_DIR}/port
    )
    target_sources(${target} PRIVATE
        ${RP_WIRELESS_DIR}/common/rp_frameq.c
        ${RP_WIRELESS_DIR}/common/rp_duty.c
    )
    target_compile_definitions(${target} PRIVATE
        RP_FRAMEQ_DEPTH=${RP_FRAMEQ_DEPTH}
    )

    if(model STREQUAL "IOT")
        set(_iot_role_args ${ARGN})
        list(LENGTH _iot_role_args _n)
        if(_n LESS 1)
            message(FATAL_ERROR "rp_configure_network(${target}): model=IOT requires an iot_role argument (LEAF|ROUTER|GATEWAY)")
        endif()
        list(GET _iot_role_args 0 iot_role)

        set(_valid_iot_roles LEAF ROUTER GATEWAY)
        if(NOT iot_role IN_LIST _valid_iot_roles)
            message(FATAL_ERROR "rp_configure_network(${target}): iot_role='${iot_role}' invalid, choose one of: ${_valid_iot_roles}")
        endif()

        target_include_directories(${target} PRIVATE
            ${RP_WIRELESS_DIR}/network/iot
            ${RP_WIRELESS_DIR}/network/iot/common
        )

        if(iot_role STREQUAL "LEAF")
            target_sources(${target} PRIVATE ${RP_WIRELESS_DIR}/network/iot/rp_iot_leaf.c)
            target_compile_definitions(${target} PRIVATE RP_LEAF_QUEUE_DEPTH=${RP_LEAF_QUEUE_DEPTH})

        elseif(iot_role STREQUAL "ROUTER")
            target_sources(${target} PRIVATE ${RP_WIRELESS_DIR}/network/iot/rp_iot_router.c)
            target_compile_definitions(${target} PRIVATE RP_MAX_NODES=${RP_ROUTER_MAX_NODES})

        elseif(iot_role STREQUAL "GATEWAY")
            target_sources(${target} PRIVATE
                ${RP_WIRELESS_DIR}/network/iot/rp_iot_gateway.c
                ${RP_WIRELESS_DIR}/network/iot/common/rp_nodetab.c
                ${RP_WIRELESS_DIR}/network/iot/common/rp_mailbox.c
            )
            target_compile_definitions(${target} PRIVATE RP_MAX_NODES=${RP_GW_MAX_NODES})
        endif()

        target_compile_definitions(${target} PRIVATE
            RP_IOT_KEEPALIVE_MS=${RP_IOT_KEEPALIVE_MS}
            RP_IOT_RX_WINDOW_MS=${RP_IOT_RX_WINDOW_MS}
        )

    else() # P2P
        target_include_directories(${target} PRIVATE ${RP_WIRELESS_DIR}/network/p2p)

        target_sources(${target} PRIVATE
            ${RP_WIRELESS_DIR}/network/p2p/rp_p2p.c
            ${RP_WIRELESS_DIR}/network/p2p/rp_neighbor.c
            ${RP_WIRELESS_DIR}/network/p2p/rp_route.c
            ${RP_WIRELESS_DIR}/network/p2p/rp_forward.c
        )
        target_compile_definitions(${target} PRIVATE
            RP_P2P_MAX_NEIGHBORS=${RP_P2P_MAX_NEIGHBORS}
            RP_P2P_MAX_ROUTES=${RP_P2P_MAX_ROUTES}
            RP_P2P_ROUTE_TIMEOUT_MS=${RP_P2P_ROUTE_TIMEOUT_MS}
            RP_P2P_DIRECT_MIN_QUALITY=${RP_P2P_DIRECT_MIN_QUALITY}
            RP_P2P_MAX_TTL=${RP_P2P_MAX_TTL}
        )
    endif()

    if(port STREQUAL "host")
        target_sources(${target} PRIVATE ${RP_WIRELESS_DIR}/port/rp_port_host.c)
    else()
        target_sources(${target} PRIVATE ${RP_WIRELESS_DIR}/port/rp_port_freertos.c)
    endif()
endfunction()
