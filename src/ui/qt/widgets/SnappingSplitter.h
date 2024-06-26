/*
* VGMTrans (c) 2002-2024
* Licensed under the zlib license,
* refer to the included LICENSE.txt file
 */

#pragma once

#include <QSplitter>
#include <QList>

class QMouseEvent;

// A SnapRange prevents the splitter from setting the size of the view to any value between the
// upper and lower bound, instead snapping to the closest bound when mouse is within the range.
struct SnapRange {
  int index;          // Index of the widget this behavior should apply to (0 or 1)
  int lowerBound;     // Lower bound for snapping
  int upperBound;     // Upper bound for snapping
};

class SnappingSplitter : public QSplitter {
public:
  explicit SnappingSplitter(Qt::Orientation orientation, QWidget* parent = nullptr);

  void enforceSnapRanges();
  void enforceSnapRangesOnResize();
  void addSnapRange(int index, int lowerBound, int upperBound);
  void clearSnapRanges();

  void persistState();
  QByteArray state;

protected:
  enum Bound: bool {
    Upper = true,
    Lower = false
  };
  void onSplitterMoved();
  void resizeEvent(QResizeEvent* event) override;
  void setSizesToBound(Bound bound, const SnapRange& range);

private:
  QList<SnapRange> snapRanges;
};
