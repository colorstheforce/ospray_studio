## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

## OpenImageIO support ##

option(ENABLE_OPENIMAGEIO "Enable OpenImageIO support" ON)

if(ENABLE_OPENIMAGEIO)
  find_package(OpenImageIO REQUIRED)
  if (NOT OpenImageIO_FOUND)
    message(STATUS "OpenImageIO not found. Using stb_image instead.")
  endif()
else()
  message(STATUS "Building without OpenVDB support. Using stb_image instead.")
endif()

## OpenVDB support ##

option(ENABLE_OPENVDB "Enable OpenVDB support" OFF)

if(ENABLE_OPENVDB)
  # Monitor <PackageName>_ROOT variables, as this mechanism is used a lot in OpenVDB.
  if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
  endif()

  # Try to use the OpenVDB-provided FindOpenVDB.cmake script.
  if (${OPENVDB_ROOT})
    list(PREPEND CMAKE_MODULE_PATH ${OPENVDB_ROOT}/lib/cmake/OpenVDB)
  endif()
  find_package(OpenVDB 7.0.0 REQUIRED)
else()
  message(STATUS "Building without OpenVDB support.")
endif()

## OpenEXR support ##

option(ENABLE_EXR "Enable EXR format image writing" ON)
if (ENABLE_EXR)
  find_package(OpenEXR REQUIRED)
else()
  message(STATUS "Building without OpenVDB support.")
endif()

## Build Library ##

add_library(ospray_sg SHARED
  Data.cpp
  Node.cpp
  Frame.cpp

  camera/Camera.cpp
  camera/Perspective.cpp
  camera/Panoramic.cpp

  exporter/Exporter.cpp
  exporter/PNG.cpp
  exporter/JPG.cpp
  exporter/PPM.cpp
  exporter/HDR.cpp
  exporter/EXR.cpp

  fb/FrameBuffer.cpp

  generator/Generator.cpp
  generator/RandomSpheres.cpp
  generator/TutorialScene.cpp
  generator/TutorialSceneML.cpp
  generator/WaveletVolume.cpp
  generator/UnstructuredVol.cpp
  generator/TestSphere.cpp
  generator/Torus.cpp

  importer/Importer.cpp
  importer/OBJ.cpp
  importer/OBJ/tiny_obj_loader_impl.cpp
  importer/glTF.cpp
  importer/glTF/tiny_gltf_impl.cpp
  importer/glTF/buffer_view.cpp
  importer/glTF/gltf_types.cpp
  importer/raw.cpp
  importer/vdb.cpp

  renderer/Material.cpp
  renderer/MaterialReference.cpp
  renderer/PathTracer.cpp
  renderer/Renderer.cpp
  renderer/SciVis.cpp
  renderer/AO.cpp
  renderer/MaterialRegistry.cpp

  renderer/materials/OBJ.cpp
  renderer/materials/Alloy.cpp
  renderer/materials/Glass.cpp
  renderer/materials/CarPaint.cpp
  renderer/materials/Luminous.cpp
  renderer/materials/Metal.cpp
  renderer/materials/MetallicPaint.cpp
  renderer/materials/Mix.cpp
  renderer/materials/Plastic.cpp
  renderer/materials/Principled.cpp
  renderer/materials/ThinGlass.cpp
  renderer/materials/Velvet.cpp

  scene/World.cpp
  scene/Animation.cpp

  scene/geometry/Geometry.cpp
  scene/geometry/Boxes.cpp
  scene/geometry/Planes.cpp
  scene/geometry/Spheres.cpp
  scene/geometry/Triangles.cpp

  scene/transfer_function/Cloud.cpp
  scene/transfer_function/Jet.cpp
  scene/transfer_function/TransferFunction.cpp

  scene/volume/Structured.cpp
  scene/volume/Unstructured.cpp
  scene/volume/StructuredSpherical.cpp
  scene/volume/Vdb.cpp
  scene/volume/Volume.cpp
  scene/volume/RawFileStructuredVolume.cpp

  scene/lights/Light.cpp
  scene/lights/Lights.cpp
  scene/lights/DirectionalLight.cpp
  scene/lights/AmbientLight.cpp
  scene/lights/SpotLight.cpp
  scene/lights/PointLight.cpp
  scene/lights/QuadLight.cpp
  scene/lights/HDRILight.cpp
  scene/lights/SunSky.cpp

  texture/Texture.cpp
  texture/Texture2D.cpp
  texture/TextureVolume.cpp
  )

target_compile_definitions(ospray_sg PUBLIC OSPRAY_CPP_RKCOMMON_TYPES)

target_link_libraries(ospray_sg
PUBLIC
  rkcommon::rkcommon
  ospray::ospray
  json
PRIVATE
  stb_image
  tiny_obj_loader
  tiny_gltf
)

install(TARGETS ospray_sg
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT lib
  # on Windows put the dlls into bin
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT lib
)

target_include_directories(ospray_sg
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
)

if (OpenImageIO_FOUND)
  target_compile_definitions(ospray_sg PRIVATE -DUSE_OPENIMAGEIO)
  target_link_libraries(ospray_sg PUBLIC OpenImageIO::OpenImageIO)
endif()

target_compile_definitions(ospray_sg
  PUBLIC
  $<$<BOOL:${ENABLE_EXR}>:STUDIO_OPENEXR>
)

if (OpenEXR_FOUND)
  target_compile_definitions(ospray_sg PRIVATE -DUSE_OPENEXR)
  target_link_libraries(ospray_sg PRIVATE OpenEXR::IlmImf)
endif()

if (OpenVDB_FOUND)
  target_compile_definitions(ospray_sg PUBLIC -DUSE_OPENVDB)
  target_link_libraries(ospray_sg PUBLIC OpenVDB::openvdb)
endif()

## Version header ##

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
  @ONLY
)

target_include_directories(ospray_sg
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

## Build Tests ##

add_subdirectory(tests)
