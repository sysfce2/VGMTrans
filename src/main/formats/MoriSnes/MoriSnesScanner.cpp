/*
 * VGMTrans (c) 2002-2024
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */

#include "MoriSnesSeq.h"
#include "ScannerManager.h"
namespace vgmtrans::scanners {
ScannerRegistration<MoriSnesScanner> s_mori_snes("MoriSnes", {"spc"});
}

//; Gokinjo Boukentai SPC
//0c3c: 1c        asl   a                 ; song index in A
//0c3d: fd        mov   y,a
//0c3e: f6 fe 11  mov   a,$11fe+y
//0c41: c4 04     mov   $04,a
//0c43: f6 ff 11  mov   a,$11ff+y
//0c46: c4 05     mov   $05,a             ; set song header ptr
//0c48: 8d 00     mov   y,#$00
//0c4a: f7 04     mov   a,($04)+y         ; read first byte
//0c4c: 10 05     bpl   $0c53
//0c4e: 68 ff     cmp   a,#$ff
//0c50: d0 14     bne   $0c66
//0c52: 6f        ret
BytePattern MoriSnesScanner::ptnLoadSeq(
	"\x1c\xfd\xf6\xfe\x11\xc4\x04\xf6"
	"\xff\x11\xc4\x05\x8d\x00\xf7\x04"
	"\x10\x05\x68\xff\xd0\x14\x6f"
	,
	"xxx??x?x"
	"??x?xxx?"
	"xxxxx?x"
	,
	23);

//; Gokinjo Bouken Tai
//02f1: 8f 11 f3  mov   $f3,#$11
//02f4: 8f 6c f2  mov   $f2,#$6c
BytePattern MoriSnesScanner::ptnSetDIR(
	"\x8f\x11\xf3\x8f\x6c\xf2"
	,
	"x?xxxx"
	,
	6);

void MoriSnesScanner::scan(RawFile *file, void *info) {
  size_t nFileLength = file->size();
  if (nFileLength == 0x10000) {
    searchForMoriSnesFromARAM(file);
  } else {
    // Search from ROM unimplemented
  }
}

void MoriSnesScanner::searchForMoriSnesFromARAM(RawFile *file) {
  MoriSnesVersion version = MORISNES_NONE;
  std::string name = file->tag.hasTitle() ? file->tag.title : file->stem();

  // scan for song list table
  uint32_t ofsLoadSeq;
  uint16_t addrSongList;
  if (file->searchBytePattern(ptnLoadSeq, ofsLoadSeq)) {
    addrSongList = file->readShort(ofsLoadSeq + 3);
  } else {
    return;
  }

  // TODO: detect engine version
  version = MORISNES_STANDARD;

  // Example: Shien The Blade Chaser (Shien's Revenge)

  // TODO: guess song index
  int8_t guessedSongIndex = -1;
  if (addrSongList + 2 <= 0x10000) {
    guessedSongIndex = 1;
  }

  // scan DIR address
  uint32_t ofsSetDIR;
  uint16_t spcDirAddr = 0;
  if (file->searchBytePattern(ptnSetDIR, ofsSetDIR)) {
    spcDirAddr = file->readByte(ofsSetDIR + 1) << 8;
  }

  uint32_t addrSongHeaderPtr = addrSongList + guessedSongIndex * 2;
  if (addrSongHeaderPtr + 2 <= 0x10000) {
    uint16_t addrSongHeader = file->readShort(addrSongHeaderPtr);

    MoriSnesSeq *newSeq = new MoriSnesSeq(file, version, addrSongHeader, name);
    if (!newSeq->loadVGMFile()) {
      delete newSeq;
      return;
    }

    if (spcDirAddr != 0) {
      MoriSnesInstrSet *newInstrSet =
          new MoriSnesInstrSet(file, version, spcDirAddr, newSeq->InstrumentAddresses, newSeq->InstrumentHints);
      if (!newInstrSet->loadVGMFile()) {
        delete newInstrSet;
        return;
      }
    }
  }
}
