/**
 * VGMTrans (c) - 2002-2021
 * Licensed under the zlib license
 * See the included LICENSE for more information
 */

#pragma once
#include "root.h"

class VGMItem;
class CScanDlg;

class WinVGMRoot : public VGMRoot {
public:
  void SelectItem(VGMItem* item);
  void SelectColl(VGMColl* coll);
  void Play(void);
  void Pause(void);
  void Stop(void);
  inline bool AreWeExiting() { return bExiting; }

  virtual void UI_SetRootPtr(VGMRoot** theRoot);
  virtual void UI_PreExit();
  virtual void UI_Exit();
  virtual void UI_BeginAddRawFile(RawFile* newFile);
  virtual void UI_BeginRemoveRawFile(RawFile* targFile);

  virtual void UI_OnBeginScan();
  virtual void UI_SetScanInfo();
  virtual void UI_OnEndScan();
  virtual void UI_AddVGMFile(VGMFile* theFile);
  virtual void UI_AddVGMSeq(VGMSeq* theSeq);
  virtual void UI_AddVGMInstrSet(VGMInstrSet* theInstrSet);
  virtual void UI_AddVGMSampColl(VGMSampColl* theSampColl);
  virtual void UI_AddVGMMisc(VGMMiscFile* theMiscFile);
  virtual void UI_AddVGMColl(VGMColl* theColl);
  virtual void UI_AddLogItem(LogItem* theLog);
  virtual void UI_BeginRemoveVGMFile(VGMFile* targFile);
  virtual void UI_RemoveVGMColl(VGMColl* targColl);
  virtual void UI_BeginRemoveVGMFiles();
  virtual void UI_EndRemoveVGMFiles();
  virtual void UI_AddItem(VGMItem* item, VGMItem* parent, const std::wstring& itemName,
                          VOID* UI_specific);
  virtual std::wstring UI_GetOpenFilePath(const std::wstring& suggestedFilename = L"",
                                          const std::wstring& extension = L"");
  virtual std::wstring UI_GetSaveFilePath(const std::wstring& suggestedFilename,
                                          const std::wstring& extension = L"");
  virtual std::wstring UI_GetSaveDirPath(const std::wstring& suggestedDir = L"");

private:
  CScanDlg* scanDlg{};
  VGMItem* selectedItem{};
  VGMColl* selectedColl{};
  VGMColl* loadedColl{};
  bool bClosingVGMFile{false};
  bool bExiting{false};
};

extern WinVGMRoot winroot;
