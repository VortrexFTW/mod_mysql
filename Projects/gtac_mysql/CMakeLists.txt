disable_rtti()

INCLUDE_DIRECTORIES("../mysql/include")
LINK_DIRECTORIES("../mysql/lib/${ENGINE_PLATFORM}")

file(GLOB_RECURSE SOURCES *.cpp *.h)

if(MSVC)
	precompiled_header(pch)
endif()

new_library_shared(gtac_mysql "${SOURCES}" "")
if(WIN32)
target_link_libraries(gtac_mysql libmysql)
else()
target_link_libraries(gtac_mysql mysql)
endif()
set_target_properties(gtac_mysql PROPERTIES FOLDER modules)
