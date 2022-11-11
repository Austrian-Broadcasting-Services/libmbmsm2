cmake_minimum_required(VERSION 3.16)

project (libmbmsifs VERSION 0.8.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_DEBUG_INIT "-Wall -Wextra -Werror -g3")
set(CMAKE_CXX_FLAGS_RELEASE_INIT "-Wall -O3")

find_package(Boost REQUIRED)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(USRSCTP REQUIRED IMPORTED_TARGET usrsctp)
#find_package(OpenSSL REQUIRED)
#pkg_check_modules(TINYXML REQUIRED IMPORTED_TARGET tinyxml2)
#pkg_check_modules(NETLINK REQUIRED IMPORTED_TARGET libnl-3.0)

add_subdirectory(ASN.1)
add_subdirectory(examples)

set(m2ap_gen_dir ${CMAKE_BINARY_DIR}/m2ap_asn1_gen)
set(m2ap_asn1_dir ${CMAKE_SOURCE_DIR}/ASN.1/M2AP)
set(m2ap_asn1
   ${m2ap_asn1_dir}/common.asn
   ${m2ap_asn1_dir}/constants.asn
   ${m2ap_asn1_dir}/container.asn
   ${m2ap_asn1_dir}/elementary.asn
   ${m2ap_asn1_dir}/information_element.asn
   ${m2ap_asn1_dir}/pdu_definitions.asn
)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${m2ap_asn1}
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${m2ap_gen_dir}
    COMMAND asn1c -fline-refs -fcompound-names -no-gen-BER -no-gen-XER 
               -no-gen-OER -no-gen-UPER -no-gen-example -fno-include-deps
               -D ${m2ap_gen_dir}
               ${m2ap_asn1}
               WORKING_DIRECTORY ${m2ap_asn1_dir}
)

file(GLOB_RECURSE m2ap_gen_files ${m2ap_gen_dir} *.c *.h)

include_directories(
    "${PROJECT_BINARY_DIR}"
    ${PROJECT_SOURCE_DIR}/include

    SYSTEM
    ${m2ap_gen_dir}
    ${SPDLOG_INCLUDEDIR}
    PkgConfig::USRSCTP
    )

configure_file("include/Version.h.in" "Version.h")

link_directories(
    )

#set(CMAKE_CXX_CLANG_TIDY clang-tidy)

add_library(mbmsifs "")
target_sources(mbmsifs
  PRIVATE
  src/M2Server.cpp
  src/M2ServerConnection.cpp
  src/M2Client.cpp
  ${m2ap_gen_files}

  PUBLIC
  include/M2Server.h
  include/M2Client.h
  )

target_include_directories(mbmsifs
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include/
  )

target_link_libraries( mbmsifs
    LINK_PUBLIC
    spdlog::spdlog
    z
    pthread
    ${USRSCTP_LINK_LIBRARIES}
)

