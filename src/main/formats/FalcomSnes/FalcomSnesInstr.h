#pragma once
#include "VGMInstrSet.h"
#include "VGMRgn.h"
#include "FalcomSnesFormat.h"

class FalcomSnesInstrSet;
class FalcomSnesInstr;
class FalcomSnesRgn;

// ******************
// FalcomSnesInstrSet
// ******************

class FalcomSnesInstrSet:
    public VGMInstrSet {
 public:
  friend FalcomSnesInstr;
  friend FalcomSnesRgn;

  FalcomSnesInstrSet(RawFile *file,
                     FalcomSnesVersion ver,
                     uint32_t offset,
                     uint32_t addrSampToInstrTable,
                     uint32_t spcDirAddr,
                     const std::map<uint8_t, uint16_t> &instrADSRHints,
                     const std::string &name = "FalcomSnesInstrSet");
  ~FalcomSnesInstrSet() override;

  bool parseHeader() override;
  bool parseInstrPointers() override;

  FalcomSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint32_t addrSampToInstrTable;
  std::vector<uint8_t> usedSRCNs;
  std::map<uint8_t, uint16_t> instrADSRHints;
};

// *************
// FalcomSnesInstr
// *************

class FalcomSnesInstr
    : public VGMInstr {
 public:
  FalcomSnesInstr(VGMInstrSet *instrSet,
                  FalcomSnesVersion ver,
                  uint32_t offset,
                  uint32_t theBank,
                  uint32_t theInstrNum,
                  uint8_t srcn,
                  uint32_t spcDirAddr,
                  const std::string &name = "FalcomSnesInstr");
  ~FalcomSnesInstr() override;

  bool loadInstr() override;

  FalcomSnesVersion version;

 protected:
  uint8_t srcn;
  uint32_t spcDirAddr;
};

// ***********
// FalcomSnesRgn
// ***********

class FalcomSnesRgn
    : public VGMRgn {
 public:
  FalcomSnesRgn(FalcomSnesInstr *instr,
             FalcomSnesVersion ver,
             uint32_t offset,
             uint8_t srcn);
  ~FalcomSnesRgn() override;

  bool loadRgn() override;

  FalcomSnesVersion version;
};
