project ("gstYarpDevice")

cmake_minimum_required(VERSION 3.0) 
#find_package()


find_package(PkgConfig)

pkg_check_modules(GST REQUIRED gstreamer-1.0>=1.4
                               gstreamer-sdp-1.0>=1.4
                               gstreamer-video-1.0>=1.4
                               gstreamer-app-1.0>=1.4)

pkg_check_modules(GDKPIXBUF REQUIRED gdk-pixbuf-2.0)


find_package(YARP)

if(NOT ${GDKPIXBUF_FOUND})
    message("GDKPIXBUF_FOUND == false!!!")
endif()

add_subdirectory(src)

