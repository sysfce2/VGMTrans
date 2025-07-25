/*
* VGMTrans (c) 2002-2024
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */
#include "VGMRgn.h"
#include "VGMInstrSet.h"

// ******
// VGMRgn
// ******

VGMRgn::VGMRgn(VGMInstr *instr, uint32_t offset, uint32_t length, std::string name)
    : VGMItem(instr->parInstrSet, offset, length, std::move(name)),
      parInstr(instr),
      keyLow(0),
      keyHigh(0x7F),
      velLow(0),
      velHigh(0x7F),
      unityKey(-1),
      fineTune(0),
      sampNum(0),
      sampOffset(-1),
      sampCollPtr(nullptr),
      volume(-1),
      pan(0.5),
      attack_time(0),
      attack_transform(no_transform),
      hold_time(0),
      decay_time(0),
      sustain_level(-1),
      sustain_time(0),
      release_transform(no_transform),
      release_time(0) {
}

VGMRgn::VGMRgn(VGMInstr *instr, uint32_t offset, uint32_t length, uint8_t theKeyLow, uint8_t theKeyHigh,
               uint8_t theVelLow, uint8_t theVelHigh, int theSampNum, std::string name)
    : VGMItem(instr->parInstrSet, offset, length, std::move(name)),
      parInstr(instr),
      keyLow(theKeyLow),
      keyHigh(theKeyHigh),
      velLow(theVelLow),
      velHigh(theVelHigh),
      unityKey(-1),
      fineTune(0),
      sampNum(theSampNum),
      sampOffset(-1),
      sampCollPtr(nullptr),
      volume(-1),
      pan(0.5),
      attack_time(0),
      attack_transform(no_transform),
      hold_time(0),
      decay_time(0),
      sustain_level(-1),
      sustain_time(0),
      release_transform(no_transform),
      release_time(0) {
}

void VGMRgn::setRanges(uint8_t theKeyLow, uint8_t theKeyHigh, uint8_t theVelLow, uint8_t theVelHigh) {
  keyLow = theKeyLow;
  keyHigh = theKeyHigh;
  velLow = theVelLow;
  velHigh = theVelHigh;
}

void VGMRgn::setUnityKey(uint8_t theUnityKey) {
  unityKey = theUnityKey;
}

void VGMRgn::setSampNum(uint8_t sampNumber) {
  sampNum = sampNumber;
}

void VGMRgn::setPan(uint8_t p) {
  //pan = thePan;
  pan = p;
  if (pan == 127)
    pan = 1.0;
  else if (pan == 0)
    pan = 0;
  else if (pan == 64)
    pan = 0.5;
  else
    pan = pan / static_cast<double>(127);
}

void VGMRgn::setLoopInfo(int theLoopStatus, uint32_t theLoopStart, uint32_t theLoopLength) {
  loop.loopStatus = theLoopStatus;
  loop.loopStart = theLoopStart;
  loop.loopLength = theLoopLength;
}


void VGMRgn::setADSR(long attackTime, uint16_t atkTransform, long decayTime, long sustainLev,
                     uint16_t rlsTransform, long releaseTime) {
  attack_time = attackTime;
  attack_transform = atkTransform;
  decay_time = decayTime;
  sustain_level = sustainLev;
  release_transform = rlsTransform;
  release_time = releaseTime;
}

void VGMRgn::addGeneralItem(uint32_t offset, uint32_t length, const std::string &name) {
  addChild(new VGMRgnItem(this, VGMRgnItem::RIT_GENERIC, offset, length, name));
}

void VGMRgn::addUnknown(uint32_t offset, uint32_t length) {
  addChild(new VGMRgnItem(this, VGMRgnItem::RIT_UNKNOWN, offset, length, "Unknown"));
}

//assumes pan is given as 0-127 value, converts it to our double -1.0 to 1.0 format
void VGMRgn::addPan(uint8_t p, uint32_t offset, uint32_t length, const std::string& name) {
  setPan(p);
  addChild(new VGMRgnItem(this, VGMRgnItem::RIT_PAN, offset, length, name));
}

void VGMRgn::setVolume(double vol) {
  volume = vol;
}

void VGMRgn::addVolume(double vol, uint32_t offset, uint32_t length) {
  volume = vol;
  addChild(new VGMRgnItem(this, VGMRgnItem::RIT_VOL, offset, length, "Volume"));
}

void VGMRgn::addUnityKey(uint8_t uk, uint32_t offset, uint32_t length) {
  this->unityKey = uk;
  addChild(new VGMRgnItem(this, VGMRgnItem::RIT_UNITYKEY, offset, length, "Unity Key"));
}

void VGMRgn::addFineTune(int16_t relativePitchCents, uint32_t offset, uint32_t length) {
  this->fineTune = relativePitchCents;
  addChild(new VGMRgnItem(this, VGMRgnItem::RIT_FINETUNE, offset, length, "Fine Tune"));
}

void VGMRgn::addKeyLow(uint8_t kl, uint32_t offset, uint32_t length) {
  keyLow = kl;
  addChild(new VGMRgnItem(this, VGMRgnItem::RIT_KEYLOW, offset, length, "Note Range: Low Key"));
}

void VGMRgn::addKeyHigh(uint8_t kh, uint32_t offset, uint32_t length) {
  keyHigh = kh;
  addChild(new VGMRgnItem(this, VGMRgnItem::RIT_KEYHIGH, offset, length, "Note Range: High Key"));
}

void VGMRgn::addVelLow(uint8_t vl, uint32_t offset, uint32_t length) {
  velLow = vl;
  addChild(new VGMRgnItem(this, VGMRgnItem::RIT_VELLOW, offset, length, "Vel Range: Low"));
}

void VGMRgn::addVelHigh(uint8_t vh, uint32_t offset, uint32_t length) {
  velHigh = vh;
  addChild(new VGMRgnItem(this, VGMRgnItem::RIT_VELHIGH, offset, length, "Vel Range: High"));
}

void VGMRgn::addSampNum(int sn, uint32_t offset, uint32_t length) {
  sampNum = sn;
  addChild(new VGMRgnItem(this, VGMRgnItem::RIT_SAMPNUM, offset, length, "Sample Number"));
}

void VGMRgn::addADSRValue(uint32_t offset, uint32_t length, const std::string& name) {
  addChild(new VGMRgnItem(this, VGMRgnItem::RIT_ADSR, offset, length, name));
}


// **********
// VGMRgnItem
// **********

VGMRgnItem::VGMRgnItem(const VGMRgn *rgn, RgnItemType rgnItemType, uint32_t offset, uint32_t length, std::string name)
    : VGMItem(rgn->vgmFile(), offset, length, std::move(name), resolveType(rgnItemType)) {
}
