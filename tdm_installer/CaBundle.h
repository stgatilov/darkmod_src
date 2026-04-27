#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
	extern const int TDM_INSTALLER_CA_BUNDLE_LEN;
	extern const uint8_t TDM_INSTALLER_CA_BUNDLE_PTR[];
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
	#include <string>
	inline std::string GetCaBundle() {
		return std::string((char*)TDM_INSTALLER_CA_BUNDLE_PTR, TDM_INSTALLER_CA_BUNDLE_LEN);
	}
#endif
