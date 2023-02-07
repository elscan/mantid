# Setup for CCache
get_property(_is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT _is_multi_config)
  option(USE_CCACHE "Use ccache to cache object artifacts if available" ON)
  if(USE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
      set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
      set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    endif()
  endif()
endif()
