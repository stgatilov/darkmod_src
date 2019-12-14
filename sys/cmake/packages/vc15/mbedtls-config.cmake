set(MBEDTLS_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/ThirdParty/artefacts/mbedtls/include")
set(MBEDTLS_LIBRARY_DIR "${PROJECT_SOURCE_DIR}/ThirdParty/artefacts/mbedtls/lib/${PACKAGE_PLATFORM}")
set(MBEDTLS_LIBRARIES
    "${MBEDTLS_LIBRARY_DIR}/mbedcrypto.lib"
    "${MBEDTLS_LIBRARY_DIR}/mbedtls.lib"
    "${MBEDTLS_LIBRARY_DIR}/mbedx509.lib"
)
