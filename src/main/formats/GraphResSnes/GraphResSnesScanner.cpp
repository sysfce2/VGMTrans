/*
 * VGMTrans (c) 2002-2024
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */

#include "GraphResSnesSeq.h"
#include "GraphResSnesInstr.h"
#include "ScannerManager.h"

namespace vgmtrans::scanners {
ScannerRegistration<GraphResSnesScanner> s_graph_snes("GraphResSnes", {"spc"});
}

//; Mickey no Tokyo Disneyland Daibouken SPC
//0620: 3f 24 05  call  $0524
//0623: e8 00     mov   a,#$00
//0625: c4 0d     mov   $0d,a
//0627: e8 12     mov   a,#$12
//0629: c4 0e     mov   $0e,a             ; song header at $1200
//062b: e5 01 12  mov   a,$1201           ; $1201/2: first track address (ROM address, not APU RAM address)
//062e: 80        setc
//062f: a8 18     sbc   a,#$18
//0631: c4 0f     mov   $0f,a
//0633: e5 02 12  mov   a,$1202
//0636: a8 00     sbc   a,#$00
//0638: c4 10     mov   $10,a             ; $0f/10 = first track address - 24 header bytes
BytePattern GraphResSnesScanner::ptnLoadSeq(
	"\x3f\x24\x05\xe8\x00\xc4\x0d\xe8"
	"\x12\xc4\x0e\xe5\x01\x12\x80\xa8"
	"\x18\xc4\x0f\xe5\x02\x12\xa8\x00"
	"\xc4\x10"
	,
	"x??xxx?x"
	"?x?x??xx"
	"xx?x??x?"
	"x?"
	,
	26);

//; Mickey no Tokyo Disneyland Daibouken SPC
//0575: 8d 00     mov   y,#$00
//0577: f6 d0 05  mov   a,$05d0+y
//057a: 5d        mov   x,a
//057b: f6 cf 05  mov   a,$05cf+y
//057e: 68 ff     cmp   a,#$ff
//0580: f0 07     beq   $0589
//0582: 3f 62 0a  call  $0a62             ; reset DSP regs
//0585: fc        inc   y
//0586: fc        inc   y
BytePattern GraphResSnesScanner::ptnDspRegInit(
	"\x8d\x00\xf6\xd0\x05\x5d\xf6\xcf"
	"\x05\x68\xff\xf0\x07\x3f\x62\x0a"
	"\xfc\xfc"
	,
	"xxx??xx?"
	"?xxxxx??"
	"xx"
	,
	18);

void GraphResSnesScanner::scan(RawFile *file, void *info) {
  size_t nFileLength = file->size();
  if (nFileLength == 0x10000) {
    searchForGraphResSnesFromARAM(file);
  } else {
    // Search from ROM unimplemented
  }
}

void GraphResSnesScanner::searchForGraphResSnesFromARAM(RawFile *file) {
  std::string name = file->tag.hasTitle() ? file->tag.title : file->stem();

  // search song header
  uint32_t ofsLoadSeq;
  uint16_t addrSeqHeader;
  if (file->searchBytePattern(ptnLoadSeq, ofsLoadSeq)) {
    addrSeqHeader = file->readByte(ofsLoadSeq + 4) | (file->readByte(ofsLoadSeq + 8) << 8);
  } else {
    return;
  }

  GraphResSnesVersion version = GRAPHRESSNES_STANDARD;

  GraphResSnesSeq *newSeq = new GraphResSnesSeq(file, version, addrSeqHeader, name);
  if (!newSeq->loadVGMFile()) {
    delete newSeq;
    return;
  }

  // get sample map address from DIR register value
  std::map<uint8_t, uint8_t> dspRegMap = getInitDspRegMap(file);
  std::map<uint8_t, uint8_t>::iterator itSpcDIR = dspRegMap.find(0x5d);
  if (itSpcDIR == dspRegMap.end()) {
    return;
  }
  uint16_t spcDirAddr = itSpcDIR->second << 8;

  // scan SRCN table
  GraphResSnesInstrSet *newInstrSet = new GraphResSnesInstrSet(file, version, spcDirAddr, newSeq->instrADSRHints);
  if (!newInstrSet->loadVGMFile()) {
    delete newInstrSet;
    return;
  }
}

std::map<uint8_t, uint8_t> GraphResSnesScanner::getInitDspRegMap(const RawFile *file) {
  std::map<uint8_t, uint8_t> dspRegMap;

  // find a code block which initializes dsp registers
  uint32_t ofsDspRegInitASM;
  uint32_t addrDspRegList;
  if (file->searchBytePattern(ptnDspRegInit, ofsDspRegInitASM)) {
    addrDspRegList = file->readShort(ofsDspRegInitASM + 7);
  } else {
    return dspRegMap;
  }

  // store dsp reg/value pairs to map
  uint16_t curOffset = addrDspRegList;
  while (true) {
    if (curOffset + 2 > 0x10000) {
      break;
    }

    uint8_t dspReg = file->readByte(curOffset++);
    if (dspReg == 0xff) {
      break;
    }

    uint8_t dspValue = file->readByte(curOffset++);
    dspRegMap[dspReg] = dspValue;
  }

  return dspRegMap;
}
