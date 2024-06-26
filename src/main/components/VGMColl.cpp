/*
 * VGMTrans (c) 2002-2024
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */

#include "VGMColl.h"
#include "VGMSeq.h"
#include "VGMInstrSet.h"
#include "VGMSampColl.h"
#include "VGMSamp.h"
#include "VGMRgn.h"
#include "ScaleConversion.h"
#include "Root.h"
#include "DLSFile.h"
#include "SF2File.h"
#include "VGMMiscFile.h"
#include "Options.h"
#include "LogManager.h"

VGMColl::VGMColl(std::string theName) : m_name(std::move(theName)) {}

void VGMColl::removeFileAssocs() {
  if (m_seq) {
    m_seq->removeCollAssoc(this);
    m_seq = nullptr;
  }

  for (auto set : m_instrsets) {
    set->removeCollAssoc(this);
  }
  for (auto samp : m_sampcolls) {
    samp->removeCollAssoc(this);
  }
  for (auto file : m_miscfiles) {
    file->removeCollAssoc(this);
  }
}

const std::string &VGMColl::name() const {
    return m_name;
}

void VGMColl::setName(const std::string& newName) {
  m_name = newName;
}

VGMSeq *VGMColl::seq() const {
  return m_seq;
}

void VGMColl::useSeq(VGMSeq *theSeq) {
  if (theSeq != nullptr)
    theSeq->addCollAssoc(this);
  if (m_seq && (theSeq != m_seq))  // if we associated with a previous sequence
    m_seq->removeCollAssoc(this);
  m_seq = theSeq;
}

void VGMColl::addInstrSet(VGMInstrSet *theInstrSet) {
  if (theInstrSet != nullptr) {
    theInstrSet->addCollAssoc(this);
    m_instrsets.push_back(theInstrSet);
  }
}

void VGMColl::addSampColl(VGMSampColl *theSampColl) {
  if (theSampColl != nullptr) {
    theSampColl->addCollAssoc(this);
    m_sampcolls.push_back(theSampColl);
  }
}

void VGMColl::addMiscFile(VGMMiscFile *theMiscFile) {
  if (theMiscFile != nullptr) {
    theMiscFile->addCollAssoc(this);
    m_miscfiles.push_back(theMiscFile);
  }
}

bool VGMColl::load() {
  if (!loadMain())
    return false;
  pRoot->addVGMColl(this);
  return true;
}

void VGMColl::unpackSampColl(DLSFile &dls, const VGMSampColl *sampColl, std::vector<VGMSamp *> &finalSamps) {
  assert(sampColl != nullptr);

  size_t nSamples = sampColl->samples.size();
  for (size_t i = 0; i < nSamples; i++) {
    VGMSamp *samp = sampColl->samples[i];

    uint32_t bufSize;
    if (samp->ulUncompressedSize)
      bufSize = samp->ulUncompressedSize;
    else
      bufSize = static_cast<uint32_t>(ceil(samp->dataLength * samp->compressionRatio()));
    auto* uncompSampBuf = new uint8_t[bufSize];    // create a new memory space for the uncompressed wave
    samp->convertToStdWave(uncompSampBuf);            // and uncompress into that space

    uint16_t blockAlign = samp->bps / 8 * samp->channels;
    dls.addWave(1, samp->channels, samp->rate, samp->rate * blockAlign, blockAlign,
                samp->bps, bufSize, uncompSampBuf, samp->name());
    finalSamps.push_back(samp);
  }
}

void VGMColl::unpackSampColl(SynthFile &synthfile, const VGMSampColl *sampColl, std::vector<VGMSamp *> &finalSamps) {
  assert(sampColl != nullptr);

  size_t nSamples = sampColl->samples.size();
  for (size_t i = 0; i < nSamples; i++) {
    VGMSamp *samp = sampColl->samples[i];

    uint32_t bufSize;
    if (samp->ulUncompressedSize)
      bufSize = samp->ulUncompressedSize;
    else
      bufSize = static_cast<uint32_t>(ceil(samp->dataLength * samp->compressionRatio()));

    uint8_t *uncompSampBuf = new uint8_t[bufSize];    // create a new memory space for the uncompressed wave
    samp->convertToStdWave(uncompSampBuf);            // and uncompress into that space

    uint16_t blockAlign = samp->bps / 8 * samp->channels;
    SynthWave *wave = synthfile.addWave(1, samp->channels, samp->rate, samp->rate * blockAlign, blockAlign,
                                        samp->bps, bufSize, uncompSampBuf, samp->name());
    finalSamps.push_back(samp);

    // If we don't have any loop information, then don't create a sampInfo structure for the Wave
    if (samp->loop.loopStatus == -1) {
      L_ERROR("No loop information for {} - some parameters might be incorrect", samp->name());
      return;
    }

    SynthSampInfo *sampInfo = wave->addSampInfo();
    if (samp->bPSXLoopInfoPrioritizing) {
      if (samp->loop.loopStart != 0 || samp->loop.loopLength != 0)
        sampInfo->setLoopInfo(samp->loop, samp);
    } else
      sampInfo->setLoopInfo(samp->loop, samp);

    double attenuation = (samp->volume != -1) ? convertLogScaleValToAtten(samp->volume) : 0;
    uint8_t unityKey = (samp->unityKey != -1) ? samp->unityKey : 0x3C;
    short fineTune = samp->fineTune;
    sampInfo->setPitchInfo(unityKey, fineTune, attenuation);
  }
}

bool VGMColl::createDLSFile(DLSFile &dls) {
  bool result = true;
  preSynthFileCreation();
  result &= mainDLSCreation(dls);
  postSynthFileCreation();
  return result;
}

SF2File *VGMColl::createSF2File() {
  SynthFile *synthfile = createSynthFile();
  if (!synthfile) {
    L_ERROR("SF2 conversion for '{}' aborted", m_name);
    return nullptr;
  }
  SF2File *sf2file = new SF2File(synthfile);
  delete synthfile;
  return sf2file;
}

bool VGMColl::mainDLSCreation(DLSFile &dls) {
  if (m_instrsets.empty()) {
    L_ERROR("{} has no instruments", m_name);
    return false;
  }

  std::vector<VGMSamp *> finalSamps;
  std::vector<VGMSampColl *> finalSampColls;

  /* Grab samples either from the local sampcolls or from the instrument sets */
  if (!m_sampcolls.empty()) {
    for (auto & sampcoll : m_sampcolls) {
      finalSampColls.push_back(sampcoll);
      unpackSampColl(dls, sampcoll, finalSamps);
    }
  } else {
    for (auto & instrset : m_instrsets) {
      if (auto instrset_sampcoll = instrset->sampColl) {
        finalSampColls.push_back(instrset_sampcoll);
        unpackSampColl(dls, instrset_sampcoll, finalSamps);
      }
    }
  }

    if (finalSamps.empty()) {
      L_ERROR("No sample collection present for '{}'", m_name);
      return false;
    }

  for (size_t inst = 0; inst < m_instrsets.size(); inst++) {
    VGMInstrSet *set = m_instrsets[inst];
    size_t nInstrs = set->aInstrs.size();
    for (size_t i = 0; i < nInstrs; i++) {
      VGMInstr *vgminstr = set->aInstrs[i];
      size_t nRgns = vgminstr->regions().size();
      std::string name = vgminstr->name();
      auto bank_no = vgminstr->bank;
      /*
      * The ulBank field follows this structure:
      * F|00000000000000000|CC0|0|CC32
      * where F = 0 if the instrument is melodic, 1 otherwise
      * (length of each CC is 7 bits, obviously)
      */
      if (auto bs = ConversionOptions::the().bankSelectStyle(); bs == BankSelectStyle::GS) {
        bank_no &= 0x7f;
        bank_no = bank_no << 8;
      } else if (bs == BankSelectStyle::MMA) {
        const uint8_t bank_msb = (bank_no >> 7) & 0x7f;
        const uint8_t bank_lsb = bank_no & 0x7f;
        bank_no = (bank_msb << 8) | bank_lsb;
      }
      DLSInstr *newInstr = dls.addInstr(bank_no, vgminstr->instrNum, name);
      for (uint32_t j = 0; j < nRgns; j++) {
        VGMRgn *rgn = vgminstr->regions()[j];
        //				if (rgn->sampNum+1 > sampColl->samples.size())	//does thereferenced sample exist?
        //					continue;

        // Determine the SampColl associated with this rgn.  If there's an explicit pointer to it, use that.
        VGMSampColl *sampColl = rgn->sampCollPtr;
        if (!sampColl) {
          // If rgn is of an InstrSet with an embedded SampColl, use that SampColl.
          if (static_cast<VGMInstrSet*>(rgn->vgmFile())->sampColl)
            sampColl = static_cast<VGMInstrSet*>(rgn->vgmFile())->sampColl;

            // If that does not exist, assume the first SampColl
          else
            sampColl = finalSampColls[0];
        }

        // Determine the sample number within the rgn's associated SampColl
        size_t realSampNum;
        // If a sample offset is provided, then find the sample number based on this offset.
        // see sampOffset declaration in header file for more info.
        if (rgn->sampOffset != -1) {
          bool bFoundIt = false;
          for (uint32_t s = 0; s < sampColl->samples.size(); s++) {             //for every sample
            if (rgn->sampOffset == sampColl->samples[s]->dwOffset - sampColl->dwOffset - sampColl->sampDataOffset) {
              realSampNum = s;

              //samples[m]->loop.loopStart = parInstrSet->aInstrs[i]->aRgns[k]->loop.loopStart;
              //samples[m]->loop.loopLength = (samples[m]->dataLength) - (parInstrSet->aInstrs[i]->aRgns[k]->loop.loopStart); //[aInstrs[i]->aRegions[k]->sample_num]->dwUncompSize/2) - ((aInstrs[i]->aRegions[k]->loop_point*28)/16); //to end of sample
              bFoundIt = true;
              break;
            }
          }
          if (!bFoundIt) {
            L_ERROR("Failed matching region to a sample with offset {:#x} (Instrset "
                    "{}, Instr {}, Region {})", rgn->sampOffset, inst, i, j);
            realSampNum = 0;
          }
        }
          // Otherwise, the sample number should be explicitly defined in the rgn.
        else
          realSampNum = rgn->sampNum;

        // Determine the sampCollNum (index into our finalSampColls vector)
        unsigned int sampCollNum = 0;
        for (unsigned int k = 0; k < finalSampColls.size(); k++) {
          if (finalSampColls[k] == sampColl)
            sampCollNum = k;
        }
        // now we add the number of samples from the preceding SampColls to the value to
        // get the real sampNum in the final DLS file.
        for (unsigned int k = 0; k < sampCollNum; k++)
          realSampNum += finalSampColls[k]->samples.size();

        // For collections with multiple SampColls
        // If a SampColl ptr is given, find the SampColl and adjust the sample number of the region
        // to compensate for all preceding SampColl samples.
        //if (rgn->sampCollNum == -1)	//if a sampCollPtr is defined
        //{
        //	// find the sampColl's index in samplecolls (the sampCollNum, effectively)
        //	for (uint32_t i=0; i < finalSampColls.size(); i++)
        //	{
        //		if (finalSampColls[i] == sampColl)
        //			rgn->sampCollNum = i;
        //	}
        //}
        //if (rgn->sampCollNum != -1)		//if a sampCollNum is defined
        //{									//then sampNum represents the sample number in the specific sample collection
        //	for (int k=0; k < rgn->sampCollNum; k++)
        //		realSampNum += finalSampColls[k]->samples.size();	//so now we add all previous sample collection samples to the value to get the real (absolute) sampNum
        //}

        if (realSampNum >= finalSamps.size()) {
          L_ERROR("Sample {} does not exist", realSampNum);
          realSampNum = finalSamps.size() - 1;
        }

        DLSRgn *newRgn = newInstr->addRgn();
        newRgn->setRanges(rgn->keyLow, rgn->keyHigh, rgn->velLow, rgn->velHigh);
        newRgn->setWaveLinkInfo(0, 0, 1, static_cast<uint32_t>(realSampNum));

        VGMSamp *samp = finalSamps[realSampNum]; //sampColl->samples[rgn->sampNum];
        DLSWsmp *newWsmp = newRgn->addWsmp();

        // This is a really loopy way of determining the loop information, pardon the pun.  However, it works.
        // There might be a way to simplify this, but I don't want to test out whether another method breaks anything just yet
        // Use the sample's loopStatus to determine if a loop occurs.  If it does, see if the sample provides loop info
        // (gathered during ADPCM > PCM conversion.  If the sample doesn't provide loop offset info, then use the region's
        // loop info.
        if (samp->bPSXLoopInfoPrioritizing) {
          if (samp->loop.loopStatus != -1) {
            if (samp->loop.loopStart != 0 || samp->loop.loopLength != 0)
              newWsmp->setLoopInfo(samp->loop, samp);
            else {
              rgn->loop.loopStatus = samp->loop.loopStatus;
              newWsmp->setLoopInfo(rgn->loop, samp);
            }
          } else
              throw;
        }
          // The normal method: First, we check if the rgn has loop info defined.
          // If it doesn't, then use the sample's loop info.
        else if (rgn->loop.loopStatus == -1) {
          if (samp->loop.loopStatus != -1)
            newWsmp->setLoopInfo(samp->loop, samp);
          else
            throw;
        } else
          newWsmp->setLoopInfo(rgn->loop, samp);

        int8_t realUnityKey;
        if (rgn->unityKey == -1)
          realUnityKey = samp->unityKey;
        else
          realUnityKey = rgn->unityKey;
        if (realUnityKey == -1)
          realUnityKey = 0x3C;

        short realFineTune;
        if (rgn->fineTune == 0)
          realFineTune = samp->fineTune;
        else
          realFineTune = rgn->fineTune;

        long realAttenuation;
        if (rgn->volume == -1 && samp->volume == -1)
          realAttenuation = 0;
        else if (rgn->volume == -1)
          realAttenuation = static_cast<long>(-(convertLogScaleValToAtten(samp->volume) * DLS_DECIBEL_UNIT * 10));
        else
          realAttenuation = static_cast<long>(-(convertLogScaleValToAtten(rgn->volume) * DLS_DECIBEL_UNIT * 10));

        long convAttack = static_cast<long>(std::round(secondsToTimecents(rgn->attack_time) * 65536));
        long convHold = static_cast<long>(std::round(secondsToTimecents(rgn->hold_time) * 65536));
        long convDecay = static_cast<long>(std::round(secondsToTimecents(rgn->decay_time) * 65536));
        long convSustainLev;
        if (rgn->sustain_level == -1)
          convSustainLev = 0x03e80000;        //sustain at full if no sustain level provided
        else {
          // the DLS envelope is a range from 0 to -96db.
          double attenInDB = convertLogScaleValToAtten(rgn->sustain_level);
          convSustainLev = static_cast<long>(((96.0 - attenInDB) / 96.0) * 0x03e80000);
        }

        long convRelease = static_cast<long>(std::round(secondsToTimecents(rgn->release_time) * 65536));

        DLSArt *newArt = newRgn->addArt();
        newArt->addPan(convertPercentPanTo10thPercentUnits(rgn->pan) * 65536);
        newArt->addADSR(convAttack, 0, convHold, convDecay, convSustainLev, convRelease, 0);

        newWsmp->setPitchInfo(realUnityKey, realFineTune, realAttenuation);
      }
    }
  }
  return true;
}

SynthFile *VGMColl::createSynthFile() {
  if (m_instrsets.empty()) {
    L_ERROR("{} has no instruments", m_name);
    return nullptr;
  }

  preSynthFileCreation();

  /* FIXME: shared_ptr eventually */
  SynthFile *synthfile = new SynthFile("SynthFile");

  std::vector<VGMSamp *> finalSamps;
  std::vector<VGMSampColl *> finalSampColls;

  /* Grab samples either from the local sampcolls or from the instrument sets */
  if (!m_sampcolls.empty()) {
    for (auto & sampcoll : m_sampcolls) {
      finalSampColls.push_back(sampcoll);
      unpackSampColl(*synthfile, sampcoll, finalSamps);
    }
  } else {
    for (auto & instrset : m_instrsets) {
      if (auto instrset_sampcoll = instrset->sampColl) {
        finalSampColls.push_back(instrset_sampcoll);
        unpackSampColl(*synthfile, instrset_sampcoll, finalSamps);
      }
    }
  }

  if (finalSamps.empty()) {
    L_ERROR("No sample collection present for '{}'", m_name);
    delete synthfile;
    postSynthFileCreation();
    return nullptr;
  }

  for (size_t inst = 0; inst < m_instrsets.size(); inst++) {
    VGMInstrSet *set = m_instrsets[inst];
    size_t nInstrs = set->aInstrs.size();
    for (size_t i = 0; i < nInstrs; i++) {
      VGMInstr *vgminstr = set->aInstrs[i];
      size_t nRgns = vgminstr->regions().size();
      if (nRgns == 0)  // do not write an instrument if it has no regions
        continue;
      SynthInstr *newInstr = synthfile->addInstr(vgminstr->bank, vgminstr->instrNum, vgminstr->reverb);
      for (uint32_t j = 0; j < nRgns; j++) {
        VGMRgn *rgn = vgminstr->regions()[j];
        //				if (rgn->sampNum+1 > sampColl->samples.size())	//does thereferenced sample exist?
        //					continue;

        // Determine the SampColl associated with this rgn.  If there's an explicit pointer to it, use that.
        VGMSampColl *sampColl = rgn->sampCollPtr;
        if (!sampColl) {
          // If rgn is of an InstrSet with an embedded SampColl, use that SampColl.
          if (static_cast<VGMInstrSet*>(rgn->vgmFile())->sampColl)
            sampColl = static_cast<VGMInstrSet*>(rgn->vgmFile())->sampColl;

            // If that does not exist, assume the first SampColl
          else
            sampColl = finalSampColls[0];
        }

        // Determine the sample number within the rgn's associated SampColl
        size_t realSampNum;
        // If a sample offset is provided, then find the sample number based on this offset.
        // see sampOffset declaration in header file for more info.
        if (rgn->sampOffset != -1) {
          bool bFoundIt = false;
          for (uint32_t s = 0; s < sampColl->samples.size(); s++) {  //for every sample
            if (rgn->sampOffset == sampColl->samples[s]->dwOffset - sampColl->dwOffset - sampColl->sampDataOffset) {
              realSampNum = s;

              //samples[m]->loop.loopStart = parInstrSet->aInstrs[i]->aRgns[k]->loop.loopStart;
              //samples[m]->loop.loopLength = (samples[m]->dataLength) - (parInstrSet->aInstrs[i]->aRgns[k]->loop.loopStart); //[aInstrs[i]->aRegions[k]->sample_num]->dwUncompSize/2) - ((aInstrs[i]->aRegions[k]->loop_point*28)/16); //to end of sample
              bFoundIt = true;
              break;
            }
          }
          if (!bFoundIt) {
            L_ERROR("Failed matching region to a sample with offset {:#x} (Instrset "
                    "{}, Instr {}, Region {})", rgn->sampOffset, inst, i, j);
            realSampNum = 0;
          }
        }
          // Otherwise, the sample number should be explicitly defined in the rgn.
        else
          realSampNum = rgn->sampNum;

          // Determine the sampCollNum (index into our finalSampColls vector)
        auto sampCollNum = finalSampColls.size();
        for (size_t k = 0; k < finalSampColls.size(); k++) {
          if (finalSampColls[k] == sampColl)
            sampCollNum = k;
        }
        if (sampCollNum == finalSampColls.size()) {
          L_ERROR("SampColl does not exist");
          postSynthFileCreation();
          return nullptr;
        }
        //   now we add the number of samples from the preceding SampColls to the value to
        //   get the real sampNum in the final DLS file.
        for (uint32_t k = 0; k < sampCollNum; k++)
          realSampNum += finalSampColls[k]->samples.size();

        SynthRgn *newRgn = newInstr->addRgn();
        newRgn->setRanges(rgn->keyLow, rgn->keyHigh, rgn->velLow, rgn->velHigh);
        newRgn->setWaveLinkInfo(0, 0, 1, static_cast<uint32_t>(realSampNum));

        if (realSampNum >= finalSamps.size()) {
          L_ERROR("Sample {} does not exist", realSampNum);
          realSampNum = finalSamps.size() - 1;
        }

        VGMSamp *samp = finalSamps[realSampNum];  // sampColl->samples[rgn->sampNum];
        SynthSampInfo *sampInfo = newRgn->addSampInfo();

        // This is a really loopy way of determining the loop information, pardon the pun.  However, it works.
        // There might be a way to simplify this, but I don't want to test out whether another method breaks anything just yet
        // Use the sample's loopStatus to determine if a loop occurs.  If it does, see if the sample provides loop info
        // (gathered during ADPCM > PCM conversion.  If the sample doesn't provide loop offset info, then use the region's
        // loop info.
        if (samp->bPSXLoopInfoPrioritizing) {
          if (samp->loop.loopStatus != -1) {
            if (samp->loop.loopStart != 0 || samp->loop.loopLength != 0)
              sampInfo->setLoopInfo(samp->loop, samp);
            else {
              rgn->loop.loopStatus = samp->loop.loopStatus;
              sampInfo->setLoopInfo(rgn->loop, samp);
            }
          } else {
            delete synthfile;
            throw;
          }
        }
        // The normal method: First, we check if the rgn has loop info defined.
        // If it doesn't, then use the sample's loop info.
        else if (rgn->loop.loopStatus == -1) {
          if (samp->loop.loopStatus != -1)
            sampInfo->setLoopInfo(samp->loop, samp);
          else {
            delete synthfile;
            throw;
          }
        } else
          sampInfo->setLoopInfo(rgn->loop, samp);

        int8_t realUnityKey;
        if (rgn->unityKey == -1)
          realUnityKey = samp->unityKey;
        else
          realUnityKey = rgn->unityKey;
        if (realUnityKey == -1)
          realUnityKey = 0x3C;

        short realFineTune;
        if (rgn->fineTune == 0)
          realFineTune = samp->fineTune;
        else
          realFineTune = rgn->fineTune;

        double attenuation;
        if (rgn->volume != -1)
          attenuation = convertLogScaleValToAtten(rgn->volume);
        else if (samp->volume != -1)
          attenuation = convertLogScaleValToAtten(samp->volume);
        else
          attenuation = 0;

        sampInfo->setPitchInfo(realUnityKey, realFineTune, attenuation);

        double sustainLevAttenDb;
        if (rgn->sustain_level == -1)
          sustainLevAttenDb = 0.0;
        else
          sustainLevAttenDb = convertPercentAmplitudeToAttenDB_SF2(rgn->sustain_level);

        SynthArt *newArt = newRgn->addArt();
        newArt->addPan(rgn->pan);
        newArt->addADSR(rgn->attack_time, static_cast<Transform>(rgn->attack_transform),
          rgn->hold_time, rgn->decay_time, sustainLevAttenDb, rgn->sustain_time, rgn->release_time,
          static_cast<Transform>(rgn->release_transform));
      }
    }
  }
  postSynthFileCreation();
  return synthfile;
}

// A helper lambda function to search for a file in a vector of VGMFile*
template<typename T>
bool contains(const std::vector<T*>& vec, const VGMFile* file) {
  return std::any_of(vec.begin(), vec.end(), [file](const VGMFile* elem) {
    return elem == file;
  });
}

bool VGMColl::containsVGMFile(const VGMFile* file) const {
  // First, check if the file matches the seq property directly
  if (m_seq == file) {
    return true;
  }

  // Then, check if the file is present in any of the file vectors
  if (contains(m_instrsets, file) || contains(m_sampcolls, file) || contains(m_miscfiles, file)) {
    return true;
  }
  return false;
}