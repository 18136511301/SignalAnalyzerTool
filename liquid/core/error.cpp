#include "error.h"

namespace liquid {
namespace core {

const char* error_string(ErrorCode code) {
    switch (code) {
        case LIQUID_OK:        return "ok";
        case LIQUID_EINT:      return "internal error";
        case LIQUID_EIOBJ:     return "invalid object";
        case LIQUID_EICONFIG:  return "invalid configuration";
        case LIQUID_EIVAL:     return "invalid value";
        case LIQUID_EIRANGE:   return "invalid range";
        case LIQUID_EIMODE:    return "invalid mode";
        case LIQUID_EUMODE:    return "unsupported mode";
        case LIQUID_ENOINIT:   return "object not initialized";
        case LIQUID_EIMEM:     return "invalid memory";
        case LIQUID_EIO:       return "i/o error";
        case LIQUID_ENOCONV:   return "algorithm did not converge";
        case LIQUID_ENOIMP:    return "not implemented";
        default:               return "unknown error";
    }
}

} // namespace core
} // namespace liquid
