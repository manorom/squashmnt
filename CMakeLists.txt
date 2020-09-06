cmake_minimum_required(VERSION 3.0.0)
project(squashmnt C)

add_executable(squashmnt squashmnt.c mount_image.c mount_lookup.c loop.c tempdir.c)
install(TARGETS squashmnt)
