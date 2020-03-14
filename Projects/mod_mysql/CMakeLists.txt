disable_rtti()

INCLUDE_DIRECTORIES("../mysql/include")
LINK_DIRECTORIES("../mysql/lib/${ENGINE_PLATFORM}")

file(GLOB_RECURSE SOURCES *.cpp *.h)

new_library_shared(mod_mysql "${SOURCES}" "")
if(WIN32)
target_link_libraries(mod_mysql libmysql)
else()
target_link_libraries(mod_mysql mysql)
endif()
set_target_properties(mod_mysql PROPERTIES FOLDER modules)
