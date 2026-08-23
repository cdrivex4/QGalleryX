#include "BC1Engine.h"
#include <QElapsedTimer>
#include <QBuffer>
#include <algorithm>
#include <cmath>
#include <cstring>

#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) || defined(__clang__)
#include <cpuid.h>
#endif

// Fast RGB565 conversion
static inline uint16_t rgbTo565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

static inline void rgb565ToRgb(uint16_t c, uint8_t &r, uint8_t &g, uint8_t &b) {
  r = ((c >> 11) & 0x1F) * 527 + 23 >> 6;
  g = ((c >> 5) & 0x3F) * 259 + 33 >> 6;
  b = (c & 0x1F) * 527 + 23 >> 6;
}

BC1Engine::SimdLevel BC1Engine::detectSimdLevel() {
  static SimdLevel s_level = []() {
    int cpuInfo[4] = {0};
#if defined(_MSC_VER)
    __cpuid(cpuInfo, 0);
    int nIds = cpuInfo[0];
    if (nIds >= 1) {
      __cpuid(cpuInfo, 1);
      bool sse42 = (cpuInfo[2] & (1 << 20)) != 0;
      bool osxsave = (cpuInfo[2] & (1 << 27)) != 0;
      bool avx = (cpuInfo[2] & (1 << 28)) != 0;
      if (nIds >= 7 && osxsave && avx) {
        __cpuidex(cpuInfo, 7, 0);
        bool avx2 = (cpuInfo[1] & (1 << 5)) != 0;
        if (avx2) return SIMD_AVX2;
      }
      if (sse42) return SIMD_SSE42;
    }
#elif defined(__GNUC__) || defined(__clang__)
    if (__builtin_cpu_supports("avx2"))
      return SIMD_AVX2;
    if (__builtin_cpu_supports("sse4.2"))
      return SIMD_SSE42;
#endif
    return SIMD_SCALAR;
  }();
  return s_level;
}

const char *BC1Engine::simdLevelString(SimdLevel lvl) {
  switch (lvl) {
  case SIMD_AVX2:
    return "AVX2 (256-bit SIMD, Kaby Lake i3-7100+)";
  case SIMD_SSE42:
    return "SSE4.2 (128-bit SIMD, Clarkdale i3 550)";
  default:
    return "Scalar / Baseline";
  }
}

// Compress a single 4x4 block of 32-bit RGBA pixels into 8 bytes of BC1
static void compressBlock4x4(const uint32_t *srcData, int stride, int w, int h, int startX, int startY, uint8_t *dstBlock) {
  uint8_t minR = 255, minG = 255, minB = 255;
  uint8_t maxR = 0, maxG = 0, maxB = 0;

  uint8_t blockR[16], blockG[16], blockB[16];

  for (int y = 0; y < 4; ++y) {
    int py = std::min(startY + y, h - 1);
    const uint32_t *row = srcData + (py * stride);
    for (int x = 0; x < 4; ++x) {
      int px = std::min(startX + x, w - 1);
      uint32_t p = row[px];
      uint8_t r = (p >> 16) & 0xFF;
      uint8_t g = (p >> 8) & 0xFF;
      uint8_t b = p & 0xFF;

      int idx = y * 4 + x;
      blockR[idx] = r;
      blockG[idx] = g;
      blockB[idx] = b;

      minR = std::min(minR, r);
      minG = std::min(minG, g);
      minB = std::min(minB, b);

      maxR = std::max(maxR, r);
      maxG = std::max(maxG, g);
      maxB = std::max(maxB, b);
    }
  }

  uint16_t c0 = rgbTo565(maxR, maxG, maxB);
  uint16_t c1 = rgbTo565(minR, minG, minB);

  if (c0 < c1) {
    std::swap(c0, c1);
  }

  // Palette reconstruction
  uint8_t palR[4], palG[4], palB[4];
  rgb565ToRgb(c0, palR[0], palG[0], palB[0]);
  rgb565ToRgb(c1, palR[1], palG[1], palB[1]);

  if (c0 > c1) {
    palR[2] = (2 * palR[0] + palR[1] + 1) / 3;
    palG[2] = (2 * palR[0] + palR[1] + 1) / 3;
    palB[2] = (2 * palR[0] + palR[1] + 1) / 3;

    palR[3] = (palR[0] + 2 * palR[1] + 1) / 3;
    palG[3] = (palG[0] + 2 * palR[1] + 1) / 3;
    palB[3] = (palB[0] + 2 * palR[1] + 1) / 3;
  } else {
    palR[2] = (palR[0] + palR[1]) / 2;
    palG[2] = (palG[0] + palG[1]) / 2;
    palB[2] = (palB[0] + palB[1]) / 2;

    palR[3] = 0; palG[3] = 0; palB[3] = 0;
  }

  // Find best color indices for 16 pixels
  uint32_t indices = 0;
  for (int i = 0; i < 16; ++i) {
    int r = blockR[i];
    int g = blockG[i];
    int b = blockB[i];

    int bestDist = 1000000;
    int bestIdx = 0;
    int maxPal = (c0 > c1) ? 4 : 3;

    for (int p = 0; p < maxPal; ++p) {
      int dr = r - palR[p];
      int dg = g - palG[p];
      int db = b - palB[p];
      int dist = dr * dr + dg * dg + db * db;
      if (dist < bestDist) {
        bestDist = dist;
        bestIdx = p;
      }
    }
    indices |= (bestIdx << (i * 2));
  }

  // Write 8-byte block (c0: 2 bytes, c1: 2 bytes, indices: 4 bytes)
  dstBlock[0] = c0 & 0xFF;
  dstBlock[1] = (c0 >> 8) & 0xFF;
  dstBlock[2] = c1 & 0xFF;
  dstBlock[3] = (c1 >> 8) & 0xFF;
  dstBlock[4] = indices & 0xFF;
  dstBlock[5] = (indices >> 8) & 0xFF;
  dstBlock[6] = (indices >> 16) & 0xFF;
  dstBlock[7] = (indices >> 24) & 0xFF;
}

QByteArray BC1Engine::compressImage(const QImage &image) {
  if (image.isNull()) return QByteArray();

  QImage img = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
  int w = img.width();
  int h = img.height();

  // Round up to multiple of 4
  int blocksX = (w + 3) / 4;
  int blocksY = (h + 3) / 4;
  int bc1Size = blocksX * blocksY * 8;

  QByteArray result;
  result.resize(bc1Size);
  uint8_t *dst = reinterpret_cast<uint8_t *>(result.data());

  const uint32_t *srcData = reinterpret_cast<const uint32_t *>(img.constBits());
  int stride = img.bytesPerLine() / 4;

  for (int by = 0; by < blocksY; ++by) {
    for (int bx = 0; bx < blocksX; ++bx) {
      uint8_t *blockDst = dst + ((by * blocksX + bx) * 8);
      compressBlock4x4(srcData, stride, w, h, bx * 4, by * 4, blockDst);
    }
  }

  return result;
}

QImage BC1Engine::decompressImage(const QByteArray &bc1Data, int width, int height) {
  if (bc1Data.isEmpty() || width <= 0 || height <= 0) return QImage();

  int blocksX = (width + 3) / 4;
  int blocksY = (height + 3) / 4;
  if (bc1Data.size() < blocksX * blocksY * 8) return QImage();

  QImage result(width, height, QImage::Format_RGB32);
  uint32_t *dstData = reinterpret_cast<uint32_t *>(result.bits());
  int stride = result.bytesPerLine() / 4;

  const uint8_t *src = reinterpret_cast<const uint8_t *>(bc1Data.constData());

  for (int by = 0; by < blocksY; ++by) {
    for (int bx = 0; bx < blocksX; ++bx) {
      const uint8_t *block = src + ((by * blocksX + bx) * 8);

      uint16_t c0 = block[0] | (block[1] << 8);
      uint16_t c1 = block[2] | (block[3] << 8);
      uint32_t indices = block[4] | (block[5] << 8) | (block[6] << 16) | (block[7] << 24);

      uint8_t palR[4], palG[4], palB[4];
      rgb565ToRgb(c0, palR[0], palG[0], palB[0]);
      rgb565ToRgb(c1, palR[1], palG[1], palB[1]);

      if (c0 > c1) {
        palR[2] = (2 * palR[0] + palR[1] + 1) / 3;
        palG[2] = (2 * palG[0] + palG[1] + 1) / 3;
        palB[2] = (2 * palB[0] + palB[1] + 1) / 3;

        palR[3] = (palR[0] + 2 * palR[1] + 1) / 3;
        palG[3] = (palG[0] + 2 * palG[1] + 1) / 3;
        palB[3] = (palB[0] + 2 * palB[1] + 1) / 3;
      } else {
        palR[2] = (palR[0] + palR[1]) / 2;
        palG[2] = (palG[0] + palG[1]) / 2;
        palB[2] = (palB[0] + palB[1]) / 2;

        palR[3] = 0; palG[3] = 0; palB[3] = 0;
      }

      for (int y = 0; y < 4; ++y) {
        int py = by * 4 + y;
        if (py >= height) break;
        for (int x = 0; x < 4; ++x) {
          int px = bx * 4 + x;
          if (px >= width) break;

          int idxPos = (y * 4 + x) * 2;
          int colorIdx = (indices >> idxPos) & 3;

          uint32_t rgb = (0xFF << 24) |
                         (palR[colorIdx] << 16) |
                         (palG[colorIdx] << 8) |
                         palB[colorIdx];
          dstData[py * stride + px] = rgb;
        }
      }
    }
  }

  return result;
}

BC1Engine::BenchmarkResult BC1Engine::benchmarkComparison(const QList<QImage> &sampleImages) {
  BenchmarkResult res;
  if (sampleImages.isEmpty()) return res;

  // 1. Prepare JPEG buffers & measure decode time
  QList<QByteArray> jpegBuffers;
  for (const auto &img : sampleImages) {
    QByteArray buf;
    QBuffer buffer(&buf);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "JPG", 80);
    jpegBuffers.append(buf);
    res.uncompressedRamBytes += (img.width() * img.height() * 4);
  }

  QElapsedTimer timer;
  timer.start();
  for (const auto &buf : jpegBuffers) {
    QImage decoded;
    decoded.loadFromData(buf, "JPG");
  }
  res.jpegDecodeTimeMs = timer.nsecsElapsed() / 1000000.0;

  // 2. Compress to BC1 & measure encode time
  timer.restart();
  QList<QByteArray> bc1Buffers;
  for (const auto &img : sampleImages) {
    QByteArray bc1 = compressImage(img);
    bc1Buffers.append(bc1);
    res.bc1RamBytes += bc1.size();
  }
  res.bc1EncodeTimeMs = timer.nsecsElapsed() / 1000000.0;

  // 3. Measure BC1 decode time
  timer.restart();
  for (int i = 0; i < bc1Buffers.size(); ++i) {
    QImage decoded = decompressImage(bc1Buffers[i], sampleImages[i].width(), sampleImages[i].height());
  }
  res.bc1DecodeTimeMs = timer.nsecsElapsed() / 1000000.0;

  if (res.bc1RamBytes > 0) {
    res.memoryReductionRatio = (double)res.uncompressedRamBytes / res.bc1RamBytes;
  }
  if (res.bc1DecodeTimeMs > 0) {
    res.speedupRatio = res.jpegDecodeTimeMs / res.bc1DecodeTimeMs;
  }

  return res;
}
