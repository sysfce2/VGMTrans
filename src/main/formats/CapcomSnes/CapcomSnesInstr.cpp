/*
 * VGMTrans (c) 2002-2019
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */
#include "CapcomSnesInstr.h"
#include <spdlog/fmt/fmt.h>
#include "SNESDSP.h"
#include "CapcomSnesFormat.h"

// ****************
// CapcomSnesInstrSet
// ****************

CapcomSnesInstrSet::CapcomSnesInstrSet(RawFile *file, uint32_t offset, uint32_t spcDirAddr, const std::string &name) :
    VGMInstrSet(CapcomSnesFormat::name, file, offset, 0, name),
    spcDirAddr(spcDirAddr) {
}

CapcomSnesInstrSet::~CapcomSnesInstrSet() {
}

bool CapcomSnesInstrSet::parseHeader() {
  return true;
}

bool CapcomSnesInstrSet::parseInstrPointers() {
  usedSRCNs.clear();
  for (int instr = 0; instr <= 0xff; instr++) {
    uint32_t addrInstrHeader = dwOffset + (6 * instr);
    if (addrInstrHeader + 6 > 0x10000) {
      return false;
    }

    // skip blank slot (Mega Man X2, Super Street Fighter II Turbo)
    bool empty_garbage = true;
    for (uint32_t off = addrInstrHeader; off < addrInstrHeader + 6; off++) {
      const uint8_t v = readByte(off);
      if (v != 0 && v != 0xff) {
        empty_garbage = false;
        break;
      }
    }
    if (empty_garbage) {
      continue;
    }

    if (!CapcomSnesInstr::isValidHeader(this->rawFile(), addrInstrHeader, spcDirAddr, false)) {
      break;
    }
    if (!CapcomSnesInstr::isValidHeader(this->rawFile(), addrInstrHeader, spcDirAddr, true)) {
      continue;
    }

    uint8_t srcn = readByte(addrInstrHeader);
    std::vector<uint8_t>::iterator itrSRCN = std::ranges::find(usedSRCNs, srcn);
    if (itrSRCN == usedSRCNs.end()) {
      usedSRCNs.push_back(srcn);
    }

    CapcomSnesInstr *newInstr = new CapcomSnesInstr(
      this, addrInstrHeader, instr >> 7, instr & 0x7f, spcDirAddr,
      fmt::format("Instrument {}", instr));
    aInstrs.push_back(newInstr);
  }
  if (aInstrs.size() == 0) {
    return false;
  }

  std::ranges::sort(usedSRCNs);
  SNESSampColl *newSampColl = new SNESSampColl(CapcomSnesFormat::name, this->rawFile(), spcDirAddr, usedSRCNs);
  if (!newSampColl->loadVGMFile()) {
    delete newSampColl;
    return false;
  }

  return true;
}

// *************
// CapcomSnesInstr
// *************

CapcomSnesInstr::CapcomSnesInstr(VGMInstrSet *instrSet,
                                 uint32_t offset,
                                 uint32_t theBank,
                                 uint32_t theInstrNum,
                                 uint32_t spcDirAddr,
                                 const std::string &name) :
    VGMInstr(instrSet, offset, 6, theBank, theInstrNum, name),
    spcDirAddr(spcDirAddr) {
}

CapcomSnesInstr::~CapcomSnesInstr() {}

bool CapcomSnesInstr::loadInstr() {
  uint8_t srcn = readByte(dwOffset);
  uint32_t offDirEnt = spcDirAddr + (srcn * 4);
  if (offDirEnt + 4 > 0x10000) {
    return false;
  }

  uint16_t addrSampStart = readShort(offDirEnt);

  CapcomSnesRgn *rgn = new CapcomSnesRgn(this, dwOffset);
  rgn->sampOffset = addrSampStart - spcDirAddr;
  addRgn(rgn);

  return true;
}

bool CapcomSnesInstr::isValidHeader(RawFile *file, uint32_t addrInstrHeader, uint32_t spcDirAddr, bool validateSample) {
  if (addrInstrHeader + 6 > 0x10000) {
    return false;
  }

  uint8_t srcn = file->readByte(addrInstrHeader);
  uint8_t adsr1 = file->readByte(addrInstrHeader + 1);
//  uint8_t adsr2 = file->GetByte(addrInstrHeader + 2);
  uint8_t gain = file->readByte(addrInstrHeader + 3);
//  int16_t pitch_scale = file->GetShortBE(addrInstrHeader + 4);

  if (srcn >= 0x80 || (adsr1 == 0 && gain == 0)) {
    return false;
  }

  uint32_t addrDIRentry = spcDirAddr + (srcn * 4);
  if (!SNESSampColl::isValidSampleDir(file, addrDIRentry, validateSample)) {
    return false;
  }

  uint16_t srcAddr = file->readShort(addrDIRentry);
  uint16_t loopStartAddr = file->readShort(addrDIRentry + 2);
  if (srcAddr > loopStartAddr || (loopStartAddr - srcAddr) % 9 != 0) {
    return false;
  }

  return true;
}

// ***********
// CapcomSnesRgn
// ***********

CapcomSnesRgn::CapcomSnesRgn(CapcomSnesInstr *instr, uint32_t offset) :
    VGMRgn(instr, offset, 6) {
  uint8_t srcn = readByte(offset);
  uint8_t adsr1 = readByte(offset + 1);
  uint8_t adsr2 = readByte(offset + 2);
  uint8_t gain = readByte(offset + 3);
  int16_t pitch_scale = getShortBE(offset + 4);

  constexpr double pitch_fixer = 4286.0 / 4096.0;
  double fine_tuning;
  double coarse_tuning;
  fine_tuning = modf((log(pitch_scale * pitch_fixer / 256.0) / log(2.0)) * 12.0, &coarse_tuning);

  // normalize
  if (fine_tuning >= 0.5) {
    coarse_tuning += 1.0;
    fine_tuning -= 1.0;
  }
  else if (fine_tuning <= -0.5) {
    coarse_tuning -= 1.0;
    fine_tuning += 1.0;
  }

  addSampNum(srcn, offset, 1);
  addChild(offset + 1, 1, "ADSR1");
  addChild(offset + 2, 1, "ADSR2");
  addChild(offset + 3, 1, "GAIN");
  addUnityKey(96 - static_cast<int>(coarse_tuning), offset + 4, 1);
  addFineTune(static_cast<int16_t>(fine_tuning * 100.0), offset + 5, 1);
  snesConvADSR<VGMRgn>(this, adsr1, adsr2, gain);

  uint8_t sl = (adsr2 >> 5);
  emulateSDSPGAIN(gain, (sl << 8) | 0xff, 0, nullptr, &release_time);
}

CapcomSnesRgn::~CapcomSnesRgn() {
}

bool CapcomSnesRgn::loadRgn() {
  return true;
}
