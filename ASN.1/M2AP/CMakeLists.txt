set(m2ap_asn1
   common.asn
   constants.asn
   container.asn
   elementary.asn
   information_element.asn
   pdu_definitions.asn
)

set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${m2ap_asn1}
)

set(m2ap_asn_gen_dir ${CMAKE_BINARY_DIR}/m2ap_asn1_gen)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${m2ap_asn_gen_dir}
    COMMAND asn1c -fline-refs -fcompound-names -no-gen-BER -no-gen-XER 
               -no-gen-OER -no-gen-UPER -no-gen-example 
               -D ${m2ap_asn_gen_dir}
               ${m2ap_asn1}
               WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

file(GLOB_RECURSE ${generated_files} ${m2ap_asn_gen_dir} *.h *.c)
