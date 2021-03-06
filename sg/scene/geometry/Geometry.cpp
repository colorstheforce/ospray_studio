// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.h"

namespace ospray {
  namespace sg {

  Geometry::Geometry(const std::string &osp_type)
  {
    setValue(cpp::Geometry(osp_type));
    createChild("visible", "bool", true);
    createChildData("material", 0);

    child("visible").setSGOnly();
    child("material").setSGOnly();
  }

  }  // namespace sg
} // namespace ospray
