// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#if defined(USE_OPENEXR)

#include "ImageExporter.h"
// rkcommon
#include "rkcommon/os/FileName.h"
// openexr
#include "OpenEXR/ImfChannelList.h"
#include "OpenEXR/ImfOutputFile.h"

namespace ospray {
  namespace sg {

  struct EXRExporter : public ImageExporter
  {
    EXRExporter()  = default;
    ~EXRExporter() = default;

    void doExport() override;
    float *flipBuffer(const void *buf, int ncomp=4);

   private:
    void doExportAsLayers();
    void doExportAsSeparateFiles();
  };

  OSP_REGISTER_SG_NODE_NAME(EXRExporter, exporter_exr);

  // EXRExporter definitions //////////////////////////////////////////////////

  void EXRExporter::doExport()
  {
    if (child("data").valueAs<const void *>() == nullptr) {
      std::cerr << "Warning: image data null; not exporting" << std::endl;
      return;
    }

    std::string format = child("format").valueAs<std::string>();
    if (format != "float") {
      std::cerr << "Warning: saving a char buffer as EXR; image will not have "
                   "wide gamut."
                << std::endl;
      charToFloat();
    }

    if (child("asLayers").valueAs<bool>())
      doExportAsLayers();
    else
      doExportAsSeparateFiles();
  }

  void EXRExporter::doExportAsLayers()
  {
    auto file = FileName(child("file").valueAs<std::string>());

    vec2i size     = child("size").valueAs<vec2i>();
    const void *fb = child("data").valueAs<const void *>();

    // use general EXR file API for 32-bit float support
    namespace IMF   = OPENEXR_IMF_NAMESPACE;
    namespace IMATH = IMATH_NAMESPACE;

    Imf::Header exrHeader(size.x, size.y);
    exrHeader.channels().insert("R", Imf::Channel(IMF::FLOAT));
    exrHeader.channels().insert("G", Imf::Channel(IMF::FLOAT));
    exrHeader.channels().insert("B", Imf::Channel(IMF::FLOAT));
    exrHeader.channels().insert("A", Imf::Channel(IMF::FLOAT));

    auto makeSlice = [&](const void *fb, int offset, int ncomp = 4) {
      // flip the data
      return Imf::Slice(IMF::FLOAT,
                        (char *)((float *)fb + offset),
                        sizeof(float) * ncomp,
                        size.x * sizeof(float) * ncomp);
    };

    // although openexr provides a LineOrder parameter, it doesn't seem to
    // actually flip the image, so we need to do it manually.
    // This map will hold any channels we need until we write the image;
    // only then can we free the pointers. Not pretty, I know
    std::map<std::string, float *> flippedBuffers;
    flippedBuffers["fb"] = flipBuffer(fb);

    Imf::FrameBuffer exrFb;
    exrFb.insert("R", makeSlice(flippedBuffers["fb"], 0));
    exrFb.insert("G", makeSlice(flippedBuffers["fb"], 1));
    exrFb.insert("B", makeSlice(flippedBuffers["fb"], 2));
    exrFb.insert("A", makeSlice(flippedBuffers["fb"], 3));

    if (hasChild("albedo")) {
      exrHeader.channels().insert("albedo.R", Imf::Channel(IMF::FLOAT));
      exrHeader.channels().insert("albedo.G", Imf::Channel(IMF::FLOAT));
      exrHeader.channels().insert("albedo.B", Imf::Channel(IMF::FLOAT));
      const void *albedo = child("albedo").valueAs<const void *>();
      flippedBuffers["albedo"] = flipBuffer(albedo, 3);
      exrFb.insert("albedo.R", makeSlice(flippedBuffers["albedo"], 0, 3));
      exrFb.insert("albedo.G", makeSlice(flippedBuffers["albedo"], 1, 3));
      exrFb.insert("albedo.B", makeSlice(flippedBuffers["albedo"], 2, 3));
    }

    if (hasChild("Z")) {
      exrHeader.channels().insert("Z", Imf::Channel(IMF::FLOAT));
      const void *z = child("Z").valueAs<const void *>();
      flippedBuffers["Z"] = flipBuffer(z, 1);
      exrFb.insert("Z", makeSlice(flippedBuffers["Z"], 0, 1));
    }

    if (hasChild("normal")) {
      exrHeader.channels().insert("normal.X", Imf::Channel(IMF::FLOAT));
      exrHeader.channels().insert("normal.Y", Imf::Channel(IMF::FLOAT));
      exrHeader.channels().insert("normal.Z", Imf::Channel(IMF::FLOAT));
      const void *normal = child("normal").valueAs<const void *>();
      flippedBuffers["normal"] = flipBuffer(normal, 3);
      exrFb.insert("normal.X", makeSlice(flippedBuffers["normal"], 0, 3));
      exrFb.insert("normal.Y", makeSlice(flippedBuffers["normal"], 1, 3));
      exrFb.insert("normal.Z", makeSlice(flippedBuffers["normal"], 2, 3));
    }

    Imf::OutputFile exrFile(file.c_str(), exrHeader);
    exrFile.setFrameBuffer(exrFb);
    exrFile.writePixels(size.y);

    std::cout << "Saved to " << file << std::endl;

    for (auto ptr : flippedBuffers)
      free(ptr.second);
  }

  void EXRExporter::doExportAsSeparateFiles()
  {
    auto file = FileName(child("file").valueAs<std::string>());
    auto base = file.name();
    auto ext  = file.ext();

    vec2i size     = child("size").valueAs<vec2i>();
    const void *fb = child("data").valueAs<const void *>();

    // use general EXR file API for 32-bit float support
    namespace IMF   = OPENEXR_IMF_NAMESPACE;
    namespace IMATH = IMATH_NAMESPACE;

    Imf::Header beautyHeader(size.x, size.y);
    beautyHeader.channels().insert("R", Imf::Channel(IMF::FLOAT));
    beautyHeader.channels().insert("G", Imf::Channel(IMF::FLOAT));
    beautyHeader.channels().insert("B", Imf::Channel(IMF::FLOAT));
    beautyHeader.channels().insert("A", Imf::Channel(IMF::FLOAT));

    auto makeSlice = [&](const void *fb, int offset, int ncomp = 4) {
      // flip the data
      return Imf::Slice(IMF::FLOAT,
                        (char *)((float *)fb + offset),
                        sizeof(float) * ncomp,
                        size.x * sizeof(float) * ncomp);
    };

    // although openexr provides a LineOrder parameter, it doesn't seem to
    // actually flip the image, so we need to do it manually.
    // This map will hold any channels we need until we write the image;
    // only then can we free the pointers. Not pretty, I know
    std::map<std::string, float *> flippedBuffers;
    flippedBuffers["fb"] = flipBuffer(fb);

    Imf::FrameBuffer beautyFb;
    beautyFb.insert("R", makeSlice(flippedBuffers["fb"], 0));
    beautyFb.insert("G", makeSlice(flippedBuffers["fb"], 1));
    beautyFb.insert("B", makeSlice(flippedBuffers["fb"], 2));
    beautyFb.insert("A", makeSlice(flippedBuffers["fb"], 3));

    std::string beautyFilename = base + ".hdr." + ext;
    Imf::OutputFile beautyFile(beautyFilename.c_str(), beautyHeader);
    beautyFile.setFrameBuffer(beautyFb);
    beautyFile.writePixels(size.y);

    std::cout << "Saved to " << beautyFilename << std::endl;

    if (hasChild("albedo")) {
      Imf::Header albedoHeader(size.x, size.y);
      albedoHeader.channels().insert("R", Imf::Channel(IMF::FLOAT));
      albedoHeader.channels().insert("G", Imf::Channel(IMF::FLOAT));
      albedoHeader.channels().insert("B", Imf::Channel(IMF::FLOAT));
      albedoHeader.channels().insert("A", Imf::Channel(IMF::FLOAT));

      const void *albedo       = child("albedo").valueAs<const void *>();
      flippedBuffers["albedo"] = flipBuffer(albedo, 3);

      Imf::FrameBuffer albedoFb;
      albedoFb.insert("R", makeSlice(flippedBuffers["albedo"], 0, 3));
      albedoFb.insert("G", makeSlice(flippedBuffers["albedo"], 1, 3));
      albedoFb.insert("B", makeSlice(flippedBuffers["albedo"], 2, 3));
      albedoFb.insert("A", makeSlice(flippedBuffers["fb"], 3));

      std::string albedoFilename = base + ".alb." + ext;
      Imf::OutputFile albedoFile(albedoFilename.c_str(), albedoHeader);
      albedoFile.setFrameBuffer(albedoFb);
      albedoFile.writePixels(size.y);
      std::cout << "Saved to " << albedoFilename << std::endl;
    }

    if (hasChild("Z")) {
      Imf::Header depthHeader(size.x, size.y);
      depthHeader.channels().insert("R", Imf::Channel(IMF::FLOAT));
      depthHeader.channels().insert("A", Imf::Channel(IMF::FLOAT));

      const void *z       = child("Z").valueAs<const void *>();
      flippedBuffers["Z"] = flipBuffer(z, 1);

      Imf::FrameBuffer depthFb;
      depthFb.insert("R", makeSlice(flippedBuffers["Z"], 0, 1));
      depthFb.insert("A", makeSlice(flippedBuffers["fb"], 3));

      std::string depthFilename = base + ".z." + ext;
      Imf::OutputFile depthFile(depthFilename.c_str(), depthHeader);
      depthFile.setFrameBuffer(depthFb);
      depthFile.writePixels(size.y);
      std::cout << "Saved to " << depthFilename << std::endl;
    }

    if (hasChild("normal")) {
      Imf::Header normalHeader(size.x, size.y);
      normalHeader.channels().insert("R", Imf::Channel(IMF::FLOAT));
      normalHeader.channels().insert("G", Imf::Channel(IMF::FLOAT));
      normalHeader.channels().insert("B", Imf::Channel(IMF::FLOAT));
      normalHeader.channels().insert("A", Imf::Channel(IMF::FLOAT));

      const void *normal       = child("normal").valueAs<const void *>();
      flippedBuffers["normal"] = flipBuffer(normal, 3);

      Imf::FrameBuffer normalFb;
      normalFb.insert("R", makeSlice(flippedBuffers["normal"], 0, 3));
      normalFb.insert("G", makeSlice(flippedBuffers["normal"], 1, 3));
      normalFb.insert("B", makeSlice(flippedBuffers["normal"], 2, 3));
      normalFb.insert("A", makeSlice(flippedBuffers["fb"], 3));

      std::string normalFilename = base + ".nrm." + ext;
      Imf::OutputFile normalFile(normalFilename.c_str(), normalHeader);
      normalFile.setFrameBuffer(normalFb);
      normalFile.writePixels(size.y);
      std::cout << "Saved to " << normalFilename << std::endl;
    }

    for (auto ptr : flippedBuffers)
      free(ptr.second);
  }

  float *EXRExporter::flipBuffer(const void *buf, int ncomp)
  {
    vec2i size = child("size").valueAs<vec2i>();
    float *flipped =
        (float *)std::malloc(size.x * size.y * ncomp * sizeof(float));

    for (int y = size.y - 1; y >= 0; y--) {
      size_t irow = y * size.x * ncomp;
      size_t orow = (size.y - 1 - y) * size.x * ncomp;
      std::memcpy(flipped + orow,
                  ((float *)buf) + irow,
                  size.x * ncomp * sizeof(float));
    }

    return flipped;
  }

  }  // namespace sg
} // namespace ospray

#endif

