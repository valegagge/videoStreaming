add_library(gstyarpdevice SHARED gstyarpdevice.cpp gstyarpdevice.h)

include_directories(CMAKE_CURRENT_DIR  ${GST_INCLUDE_DIRS} )
include_directories(${GDKPIXBUF_INCLUDE_DIRS})
include_directories(${YARP_INCLUDE_DIRS})

#target_compile_options(gstyarpdevice ${GST_CFLAGS}) 

target_link_libraries(gstyarpdevice ${GST_LIBRARIES} ${GDKPIXBUF_LIBRARIES} ${YARP_LIBRARIES})

