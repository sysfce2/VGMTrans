/*
 * VGMTrans (c) 2002-2024
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */

#include "SonyPS2Seq.h"
#include "SonyPS2InstrSet.h"
#include "ScannerManager.h"

namespace vgmtrans::scanners {
ScannerRegistration<SonyPS2Scanner> s_sonyps2("SonyPS2", {"sq", "hd", "bd"});
}

#define SRCH_BUF_SIZE 0x20000

void SonyPS2Scanner::scan(RawFile *file, void *info) {
  searchForSeq(file);
  searchForInstrSet(file);
  searchForSampColl(file);
}

void SonyPS2Scanner::searchForSeq(RawFile *file) {
  size_t nFileLength = file->size();
  for (uint32_t i = 0; i + 0x40 < nFileLength; i++) {
    uint32_t sig1 = file->readWord(i);
    uint32_t sig2 = file->readWord(i + 4);
    if (sig1 != 0x53434549 || sig2 != 0x56657273)  // "SCEIVers" in ASCII
      continue;

    sig1 = file->readWord(i + 0x10);
    sig2 = file->readWord(i + 0x14);
    if (sig1 != 0x53434549 || sig2 != 0x53657175)  // "SCEISequ" in ASCII
      continue;

    sig1 = file->readWord(i + 0x30);
    sig2 = file->readWord(i + 0x34);
    if (sig1 != 0x53434549 || sig2 != 0x4D696469)  // "SCEIMidi" in ASCII
      continue;

    SonyPS2Seq *newSeq = new SonyPS2Seq(file, i, "Sony PS2 Seq");
    if (!newSeq->loadVGMFile())
      delete newSeq;
  }
}

void SonyPS2Scanner::searchForInstrSet(RawFile *file) {
  size_t nFileLength = file->size();
  for (uint32_t i = 0; i + 0x40 < nFileLength; i++) {
    uint32_t sig1 = file->readWord(i);
    uint32_t sig2 = file->readWord(i + 4);
    if (sig1 != 0x53434549 || sig2 != 0x56657273)  // "SCEIVers" in ASCII
      continue;

    sig1 = file->readWord(i + 0x10);
    sig2 = file->readWord(i + 0x14);
    if (sig1 != 0x53434549 || sig2 != 0x48656164)  // "SCEIHead" in ASCII
      continue;

    sig1 = file->readWord(i + 0x50);
    sig2 = file->readWord(i + 0x54);
    if (sig1 != 0x53434549 || sig2 != 0x56616769)  // "SCEIVagi" in ASCII
      continue;

    SonyPS2InstrSet *newInstrSet = new SonyPS2InstrSet(file, i);
    if (!newInstrSet->loadVGMFile())
      delete newInstrSet;
  }
}

void SonyPS2Scanner::searchForSampColl(RawFile *file) {
  if (file->size() < 32) {
    return;
  }

  if (file->extension() == "bd") {
    // Hack for incorrectly ripped bd files.  Should ALWAYS start with 16 0x00 bytes (must...
    // suppress... rage) If it doesn't, we'll throw out this file and create a new one with the
    // correct formating
    int num = std::count(file->begin(), file->begin() + 16, 0);
    if (num != 16) {
      uint32_t newFileSize = file->size() + 16;
      uint8_t *newdataBuf = new uint8_t[newFileSize];
      file->readBytes(0, file->size(), newdataBuf + 16);
      memset(newdataBuf, 0, 16);
      pRoot->createVirtFile(newdataBuf, newFileSize, file->name(), file->path().string());
      return;
    }

    SonyPS2SampColl *sampColl = new SonyPS2SampColl(file, 0);
    Format *fmt = sampColl->format();
    if (fmt)
      fmt->onNewFile(sampColl);
  }
}
