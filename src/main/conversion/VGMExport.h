/*
 * VGMTrans (c) 2002-2021
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */
#pragma once

#include "common.h"
#include "DLSFile.h"
#include "SF2File.h"
#include "VGMColl.h"
#include "VGMSeq.h"

/*
 * The following free functions implement
 * saving various formats to disk
 */

namespace conversion {

enum class Target : uint32_t { MIDI = 1u << 0u, DLS = 1u << 1u, SF2 = 1u << 2u };

inline constexpr Target operator|(Target a, Target b) {
  return static_cast<Target>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline constexpr uint32_t operator&(Target a, Target b) {
  return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
}

bool saveAsDLS(const VGMInstrSet &set, const std::string &filepath);
bool saveAsSF2(const VGMInstrSet &set, const std::string &filepath);

void saveAllAsWav(const VGMSampColl &coll, const std::string &save_dir);

bool saveAsOriginal(const VGMFile& file, const std::string& filepath);
bool saveAsOriginal(const RawFile& rawfile, const std::string& filepath);

template <Target options>
void saveAs(VGMColl &coll, const std::string &dir_path) {
  auto filename = ConvertToSafeFileName(coll.name());
  auto filepath = dir_path + "/" + filename;

  if constexpr ((options & Target::MIDI) != 0) {
    coll.seq()->saveAsMidi(filepath + ".mid");
  }

  if constexpr ((options & Target::DLS) != 0) {
    DLSFile dlsfile;
    if (coll.createDLSFile(dlsfile)) {
      dlsfile.saveDLSFile(filepath + ".dls");
    }
  }

  if constexpr ((options & Target::SF2) != 0) {
    if (SF2File *sf2file = coll.createSF2File()) {
      sf2file->saveSF2File(filepath + ".sf2");
    }
  }
}

}