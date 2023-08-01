set(YOJIMBO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../yojimbo")

add_library(yojimboLib
	"${YOJIMBO_DIR}/yojimbo.cpp" 
	"${YOJIMBO_DIR}/certs.c"
	"${YOJIMBO_DIR}/tlsf/tlsf.c"
	"${YOJIMBO_DIR}/reliable.io/reliable.c"
	"${YOJIMBO_DIR}/netcode.io/netcode.c"
)

target_include_directories(yojimboLib PUBLIC
	"${YOJIMBO_DIR}/windows"
	"${YOJIMBO_DIR}/netcode.io"
	"${YOJIMBO_DIR}/reliable.io"
)

set(YOJIMBO_LIBS_DIR "${YOJIMBO_DIR}/windows")

target_link_libraries(yojimboLib 
	"${YOJIMBO_LIBS_DIR}/sodium.lib" 
	"${YOJIMBO_LIBS_DIR}/mbedtls.lib" 
	"${YOJIMBO_LIBS_DIR}/mbedx509.lib" 
	"${YOJIMBO_LIBS_DIR}/mbedcrypto.lib" 
)