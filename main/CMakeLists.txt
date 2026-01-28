set(USER_CERT_PEM_IN "${CMAKE_CURRENT_LIST_DIR}/cert/servercert.pem")
set(USER_CERT_PEM_OUT "${CMAKE_CURRENT_BINARY_DIR}/user_cert_pem.h")

file(READ "${USER_CERT_PEM_IN}" USER_CERT_PEM_RAW)
string(REPLACE "\\" "\\\\" USER_CERT_PEM_ESC "${USER_CERT_PEM_RAW}")
string(REPLACE "\"" "\\\"" USER_CERT_PEM_ESC "${USER_CERT_PEM_ESC}")
string(REPLACE "\r\n" "\n" USER_CERT_PEM_ESC "${USER_CERT_PEM_ESC}")
string(REPLACE "\n" "\\n" USER_CERT_PEM_ESC "${USER_CERT_PEM_ESC}")

file(WRITE "${USER_CERT_PEM_OUT}" "#pragma once\n#define USER_CERT_PEM \"${USER_CERT_PEM_ESC}\"\n")

idf_component_register(
  SRCS "main.c" "optiga_demo_attestation.c"
  PRIV_REQUIRES spi_flash optiga
  INCLUDE_DIRS "${CMAKE_CURRENT_BINARY_DIR}"
)
