## Copyright 2019-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(stb_image stb_image_impl.cpp)

target_include_directories(stb_image
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
