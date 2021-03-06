// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "FrameBuffer.h"
#include "../exporter/ImageExporter.h"

namespace ospray {
  namespace sg {

  FrameBuffer::FrameBuffer()
  {
    createChild("allowDenoising",
        "bool",
        "framebuffer needs to contain format and channels compatible with denoising",
        false);
    createChild("size", "vec2i", vec2i(1024, 768));
    child("size").setReadOnly();
    createChild("colorFormat",
        "string",
        "framebuffer format: RBGA8 or float",
        std::string("RGBA8"));

    createChild("exposure", "float", "amount of light per unit area", 1.0f);
    createChild("contrast",
        "float",
        "contrast (toe of the curve); typically is in [1-2]",
        1.1759f);
    createChild("shoulder",
        "float",
        "highlight compression (shoulder of the curve); typically is in [0.9-1]",
        0.9746f);
    createChild("midIn",
        "float",
        "mid-level anchor input; default is 18\% gray",
        0.18f);
    createChild("midOut",
        "float",
        "mid-level anchor output; default is 18\% gray",
        0.18f);
    createChild(
        "hdrMax", "float", "maximum HDR input that is not clipped", 6.3704f);
    createChild("acesColor", "bool", "apply the ACES color transforms", true);

    child("exposure").setMinMax(0.f, 5.f);
    child("contrast").setMinMax(0.f, 3.f);
    child("shoulder").setMinMax(0.f, 2.f);
    child("midIn").setMinMax(0.f, 1.f);
    child("midOut").setMinMax(0.f, 1.f);
    child("hdrMax").setMinMax(0.f, 100.f);

    updateHandle();
  }

  NodeType FrameBuffer::type() const
  {
    return NodeType::FRAME_BUFFER;
  }

  const void *FrameBuffer::map(OSPFrameBufferChannel channel)
  {
    return handle().map(channel);
  }

  void FrameBuffer::unmap(const void *mem)
  {
    handle().unmap(const_cast<void *>(mem));
  }

  void FrameBuffer::resetAccumulation()
  {
    handle().clear();
  }

  float FrameBuffer::variance()
  {
    return handle().variance();
  }

  void FrameBuffer::postCommit()
  {
    updateHandle();
  }

  void FrameBuffer::updateHandle()
  {
    // Default minimal format
    child("colorFormat") = std::string("RGBA8");
    channels             = OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_VARIANCE;

    // Denoising requires float format and additional channels
    auto allowDenoising = child("allowDenoising").valueAs<bool>();
    if (allowDenoising) {
      child("colorFormat") = std::string("float");
      channels |= OSP_FB_ALBEDO | OSP_FB_DEPTH | OSP_FB_NORMAL;
    }

    auto size           = child("size").valueAs<vec2i>();
    auto colorFormatStr = child("colorFormat").valueAs<std::string>();

    auto fb = cpp::FrameBuffer(
        size.x, size.y, colorFormats[colorFormatStr], channels);

    setHandle(fb);

    // When creating/recreating the framebuffer, denoiser is initially off
    hasDenoiser = false;
  }

  void FrameBuffer::updateDenoiser(bool enabled)
  {
    // Denoiser requires float color buffer.
    if (!hasFloatFormat() || (enabled == hasDenoiser))
      return;

    hasDenoiser = enabled;
    updateImageOps = true;

    // Clear accum if changing denoiser
    handle().clear();
  }

  void FrameBuffer::updateToneMapper(bool enabled)
  {
    if (enabled == hasToneMapper)
      return;

    hasToneMapper = enabled;
    updateImageOps = true;

    // Clear accum if changing toneMapper
    handle().clear();
  }

  void FrameBuffer::updateImageOperations()
  {
    // Only update imageOperation if necessary
    if (!updateImageOps)
      return;

    updateImageOps = false; 

    std::vector<cpp::ImageOperation> ops;
    if (hasToneMapper) {
      auto iop = cpp::ImageOperation("tonemapper");
      float exposure=child("exposure").valueAs<float>();
      iop.setParam("exposure", OSP_FLOAT, &exposure);
      float contrast=child("contrast").valueAs<float>();
      iop.setParam("contrast", OSP_FLOAT, &contrast);
      float shoulder=child("shoulder").valueAs<float>();
      iop.setParam("shoulder", OSP_FLOAT, &shoulder);
      float midIn=child("midIn").valueAs<float>();
      iop.setParam("midIn", OSP_FLOAT, &midIn);
      float midOut=child("midOut").valueAs<float>();
      iop.setParam("midOut", OSP_FLOAT, &midOut);
      float hdrMax=child("hdrMax").valueAs<float>();
      iop.setParam("hdrMax", OSP_FLOAT, &hdrMax);
      bool acesColor=child("acesColor").valueAs<bool>();
      iop.setParam("acesColor", OSP_BOOL, &acesColor);
      iop.commit();
      ops.push_back(iop);
    }
    if (hasDenoiser)
      ops.push_back(cpp::ImageOperation("denoiser"));
    if (hasDenoiser || hasToneMapper)
      handle().setParam("imageOperation", cpp::CopiedData(ops));
    else
      handle().removeParam("imageOperation");
    handle().commit();
  }


  void FrameBuffer::saveFrame(std::string filename, int flags)
  {
    auto exporter = getExporter(FileName(filename));
    if (exporter == "") {
      std::cout << "No exporter found for type " << FileName(filename).ext()
                << std::endl;
      return;
    }
    auto exp = createNodeAs<ImageExporter>("exporter", exporter);
    exp->child("file") = filename;

    auto fb    = map(OSP_FB_COLOR);
    auto size  = child("size").valueAs<vec2i>();
    auto fmt   = child("colorFormat").valueAs<std::string>();
    void *abuf = nullptr;
    void *zbuf = nullptr;
    void *nbuf = nullptr;

    exp->setImageData(fb, size, fmt);

    bool albedo    = flags & 0b1;
    bool depth     = flags & 0b10;
    bool normal    = flags & 0b100;
    bool asLayers  = flags & 0b1000;

    if (albedo) {
      abuf = (void *)map(OSP_FB_ALBEDO);
      exp->setAdditionalLayer("albedo", abuf);
    } else {
      exp->clearLayer("albedo");
    }

    if (depth) {
      zbuf = (void *)map(OSP_FB_DEPTH);
      exp->setAdditionalLayer("Z", zbuf);
    } else {
      exp->clearLayer("Z");
    }

    if (normal) {
      nbuf = (void *)map(OSP_FB_NORMAL);
      exp->setAdditionalLayer("normal", nbuf);
    } else {
      exp->clearLayer("normal");
    }

    if (asLayers) {
      exp->createChild("asLayers", "bool", false);
    } else {
      exp->createChild("asLayers", "bool", true);
    }

    exp->doExport();

    unmap(fb);
    if (albedo)
      unmap(abuf);
    if (depth)
      unmap(zbuf);
    if (normal)
      unmap(nbuf);
  }

  OSP_REGISTER_SG_NODE_NAME(FrameBuffer, framebuffer);

  }  // namespace sg
} // namespace ospray
