set(Boost_USE_STATIC_LIBS TRUE)
set(components system coroutine context thread)
find_package(Boost REQUIRED ${components})

add_library(boost::boost INTERFACE IMPORTED)
set_target_properties(boost::boost PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

foreach(lib IN LISTS components)
    add_library(boost::${lib} STATIC IMPORTED)
    string(TOUPPER "${lib}" ulib)
    set_target_properties(boost::${lib} PROPERTIES
        IMPORTED_LOCATION ${Boost_${ulib}_LIBRARY_RELEASE}
        IMPORTED_LOCATION_RELEASE ${Boost_${ulib}_LIBRARY_RELEASE}
        IMPORTED_LOCATION_DEBUG ${Boost_${ulib}_LIBRARY_DEBUG}
        INTERFACE_LINK_LIBRARIES boost::boost
        )
endforeach()

set_property(TARGET boost::coroutine APPEND PROPERTY INTERFACE_LINK_LIBRARIES boost::context)

add_library(boost::variant INTERFACE IMPORTED)
set_target_properties(boost::variant PROPERTIES INTERFACE_LINK_LIBRARIES boost::boost)
set_property(TARGET boost::variant PROPERTY INTERFACE_LINK_LIBRARIES boost::boost)
add_library(boost::asio INTERFACE IMPORTED)
set_property(TARGET boost::variant PROPERTY INTERFACE_LINK_LIBRARIES boost::boost boost::system boost::coroutine boost::thread)
