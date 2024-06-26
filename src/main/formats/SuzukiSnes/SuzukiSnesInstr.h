#pragma once
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMRgn.h"
#include "SuzukiSnesFormat.h"

// ******************
// SuzukiSnesInstrSet
// ******************

class SuzukiSnesInstrSet:
    public VGMInstrSet {
 public:
  SuzukiSnesInstrSet(RawFile *file,
                     SuzukiSnesVersion ver,
                     uint32_t spcDirAddr,
                     uint16_t addrSRCNTable,
                     uint16_t addrVolumeTable,
                     uint16_t addrADSRTable,
                     uint16_t addrTuningTable,
                     const std::string &name = "SuzukiSnesInstrSet");
  virtual ~SuzukiSnesInstrSet();

  virtual bool parseHeader();
  virtual bool parseInstrPointers();

  SuzukiSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint16_t addrSRCNTable;
  uint16_t addrVolumeTable;
  uint16_t addrTuningTable;
  uint16_t addrADSRTable;
  std::vector<uint8_t> usedSRCNs;
};

// *************
// SuzukiSnesInstr
// *************

class SuzukiSnesInstr
    : public VGMInstr {
 public:
  SuzukiSnesInstr(VGMInstrSet *instrSet,
                  SuzukiSnesVersion ver,
                  uint8_t instrNum,
                  uint32_t spcDirAddr,
                  uint16_t addrSRCNTable,
                  uint16_t addrVolumeTable,
                  uint16_t addrADSRTable,
                  uint16_t addrTuningTable,
                  const std::string &name = "SuzukiSnesInstr");
  virtual ~SuzukiSnesInstr();

  virtual bool loadInstr();

  SuzukiSnesVersion version;

 protected:
  uint32_t spcDirAddr;
  uint16_t addrSRCNTable;
  uint16_t addrVolumeTable;
  uint16_t addrTuningTable;
  uint16_t addrADSRTable;
};

// *************
// SuzukiSnesRgn
// *************

class SuzukiSnesRgn
    : public VGMRgn {
 public:
  SuzukiSnesRgn(SuzukiSnesInstr *instr,
                SuzukiSnesVersion ver,
                uint8_t instrNum,
                uint32_t spcDirAddr,
                uint16_t addrSRCNTable,
                uint16_t addrVolumeTable,
                uint16_t addrADSRTable,
                uint16_t addrTuningTable);
  virtual ~SuzukiSnesRgn();

  virtual bool loadRgn();

  SuzukiSnesVersion version;
};
