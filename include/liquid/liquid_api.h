#pragma once

#ifdef LIQUID_EXPORTS
  #define LIQUID_API __declspec(dllexport)
#else
  #define LIQUID_API __declspec(dllimport)
#endif
