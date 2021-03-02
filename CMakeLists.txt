CU_SET_PATH("CMAKE_MOD_PHASED_DUELS_DIR" "${CMAKE_CURRENT_LIST_DIR}")

AC_ADD_SCRIPT("${CMAKE_CURRENT_LIST_DIR}/src/mod_phased_duels.cpp")

AC_ADD_SCRIPT_LOADER("PhasedDuels" "${CMAKE_CURRENT_LIST_DIR}/src/loader_phased_duels.h")

AC_ADD_CONFIG_FILE("${CMAKE_CURRENT_LIST_DIR}/conf/mod_phased_duels.conf.dist")
