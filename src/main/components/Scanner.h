/*
 * VGMTrans (c) 2002-2024
 * Licensed under the zlib license,
 * refer to the included LICENSE.txt file
 */
#pragma once

class RawFile;
class Format;

class VGMScanner {
 public:
  VGMScanner(Format* format);
  virtual ~VGMScanner() = default;

  virtual bool init();
  void initiateScan(RawFile *file, void *offset = nullptr);
  virtual void scan(RawFile *file, void *offset = nullptr) = 0;

protected:
  Format* format;
};
