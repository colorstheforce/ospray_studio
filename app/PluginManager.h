// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Plugin.h"

#include "sg/Frame.h"

namespace ospray {

struct PluginManager
{
  PluginManager() = default;
  ~PluginManager() = default;

  void loadPlugin(const std::string &name);
  void removePlugin(const std::string &name);

  // TODO: add functions to get a fresh set of panels, activate/deactive, etc.
  PanelList getAllPanelsFromPlugins(
      std::shared_ptr<sg::Frame> scenegraph) const;

 private:
  // Helper types //
  struct LoadedPlugin
  {
    std::unique_ptr<Plugin> instance;
    bool active{true};
  };

  // Helper functions //

  void addPlugin(std::unique_ptr<Plugin> plugin);

  // Data //

  std::vector<LoadedPlugin> plugins;
};

} // namespace ospray