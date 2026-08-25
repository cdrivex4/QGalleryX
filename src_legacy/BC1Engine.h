#ifndef BC1ENGINE_H
#define BC1ENGINE_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <QImage>
#include <QByteArray>

class BC1Engine {
public:
  enum SimdLevel {
    SIMD_SCALAR = 0,
    SIMD_SSE42  = 1,
    SIMD_AVX2   = 2
  };

  static SimdLevel detectSimdLevel();
  static const char* simdLevelString(SimdLevel lvl);

  // Compresses 32-bit ARGB/RGB32 QImage (e.g. 256x256) into 32KB BC1 raw bytes
  static QByteArray compressImage(const QImage &image);

  // Decompresses 32KB BC1 raw bytes back into 32-bit RGB32 QImage
  static QImage decompressImage(const QByteArray &bc1Data, int width, int height);

  // Micro-benchmark: measures raw throughput (megapixels/sec) for JPEG vs BC1
  struct BenchmarkResult {
    double jpegDecodeTimeMs = 0.0;
    double bc1DecodeTimeMs  = 0.0;
    double bc1EncodeTimeMs  = 0.0;
    size_t uncompressedRamBytes = 0;
    size_t bc1RamBytes = 0;
    double memoryReductionRatio = 0.0;
    double speedupRatio = 0.0;
  };

  static BenchmarkResult benchmarkComparison(const QList<QImage> &sampleImages);
};

#endif // BC1ENGINE_H
