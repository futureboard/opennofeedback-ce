// ============================================================================
//  CircularBuffer.h
//
//  Fixed-capacity, single-channel ring buffer for short delay lines / analysis
//  windows. Storage is allocated once via Resize() (call it from Prepare(),
//  never from the audio thread).
//
//  Realtime-safety note:
//    Push()/Read()/operator[] perform no allocation and no locking. Resize()
//    *does* allocate and must only be called during Prepare()/Reset().
// ============================================================================
#pragma once

#include <vector>
#include <cstddef>

namespace odf
{

class CircularBuffer
{
public:
  CircularBuffer() = default;

  /** Allocate (or re-allocate) storage. NOT realtime-safe. */
  void Resize(int capacity)
  {
    mBuffer.assign(static_cast<size_t>(capacity > 0 ? capacity : 1), 0.0f);
    mWritePos = 0;
  }

  /** Zero the contents without reallocating. Realtime-safe. */
  void Clear() noexcept
  {
    std::fill(mBuffer.begin(), mBuffer.end(), 0.0f);
    mWritePos = 0;
  }

  int Capacity() const noexcept { return static_cast<int>(mBuffer.size()); }

  /** Write one sample, advancing the head. Realtime-safe. */
  inline void Push(float x) noexcept
  {
    mBuffer[mWritePos] = x;
    if (++mWritePos >= static_cast<int>(mBuffer.size()))
      mWritePos = 0;
  }

  /** Read a sample `delay` samples in the past (delay 0 == most recent).
   *  Realtime-safe. */
  inline float Read(int delay) const noexcept
  {
    const int cap = static_cast<int>(mBuffer.size());
    int idx = mWritePos - 1 - delay;
    while (idx < 0) idx += cap;
    return mBuffer[idx];
  }

private:
  std::vector<float> mBuffer;
  int mWritePos = 0;
};

} // namespace odf
