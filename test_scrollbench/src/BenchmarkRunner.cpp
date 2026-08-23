#include "BenchmarkRunner.h"
#include "BC1Engine.h"
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtConcurrent>
#include <QDebug>
#include <iostream>
#include <iomanip>

BenchmarkRunner::BenchmarkRunner(QObject *parent) : QObject(parent) {}

void BenchmarkRunner::runDriveBenchmark(const QString &rootPath, int targetWorkingSet) {
  if (m_isRunning) return;

  m_isRunning = true;
  m_statusText = QString("Scanning %1 for images...").arg(rootPath);
  emit isRunningChanged();
  emit statusTextChanged();

  QtConcurrent::run([this, rootPath, targetWorkingSet]() {
    QElapsedTimer totalTimer;
    totalTimer.start();

    // 1. Gather images from root path
    QStringList imagePaths;
    QStringList filters = {"*.jpg", "*.jpeg", "*.png", "*.bmp", "*.webp"};
    QDirIterator it(rootPath, filters, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);

    while (it.hasNext() && imagePaths.size() < targetWorkingSet) {
      imagePaths.append(it.next());
    }

    // If drive has fewer images, generate synthetic variations to reach full working set
    QList<QImage> workingSet;
    workingSet.reserve(targetWorkingSet);

    int loadedCount = 0;
    for (const QString &p : imagePaths) {
      QImageReader reader(p);
      reader.setScaledSize(QSize(256, 256));
      QImage img = reader.read();
      if (!img.isNull()) {
        workingSet.append(img);
        loadedCount++;
      }
    }

    // Synthesize up to targetWorkingSet if needed
    if (workingSet.isEmpty()) {
      QImage dummy(256, 256, QImage::Format_RGB32);
      dummy.fill(Qt::blue);
      workingSet.append(dummy);
    }

    int baseCount = workingSet.size();
    while (workingSet.size() < targetWorkingSet) {
      workingSet.append(workingSet[workingSet.size() % baseCount]);
    }

    // 2. Detect CPU SIMD level
    BC1Engine::SimdLevel simd = BC1Engine::detectSimdLevel();
    const char *simdStr = BC1Engine::simdLevelString(simd);

    // 3. Run Benchmark comparison
    auto comp = BC1Engine::benchmarkComparison(workingSet);

    // 4. Calculate Sanitized Results
    int totalCount = workingSet.size();
    double uncompressedRamMb = comp.uncompressedRamBytes / (1024.0 * 1024.0);
    double bc1RamMb = comp.bc1RamBytes / (1024.0 * 1024.0);
    double avgJpegMs = (totalCount > 0) ? (comp.jpegDecodeTimeMs / totalCount) : 0.0;
    double avgBc1Ms = (totalCount > 0) ? (comp.bc1DecodeTimeMs / totalCount) : 0.0;

    QJsonObject json;
    json["workingSetThumbnails"] = totalCount;
    json["cpuSimdLevel"] = simdStr;
    json["jpegTotalDecodeTimeMs"] = comp.jpegDecodeTimeMs;
    json["jpegAvgPerImageMs"] = avgJpegMs;
    json["jpegRamMb"] = uncompressedRamMb;
    json["bc1TotalDecodeTimeMs"] = comp.bc1DecodeTimeMs;
    json["bc1AvgPerImageMs"] = avgBc1Ms;
    json["bc1RamMb"] = bc1RamMb;
    json["memoryReductionRatio"] = comp.memoryReductionRatio;
    json["speedupRatio"] = comp.speedupRatio;
    json["totalBenchmarkDurationMs"] = totalTimer.elapsed();

    // Write sanitized summary JSON
    QDir().mkpath("logs");
    QFile outFile("logs/benchmark_summary.json");
    if (outFile.open(QIODevice::WriteOnly)) {
      outFile.write(QJsonDocument(json).toJson(QJsonDocument::Indented));
      outFile.close();
    }

    // Output clean tabular console summary
    std::cout << "\n=======================================================\n";
    std::cout << "         QGalleryXBench: C:\\ Drive Benchmark           \n";
    std::cout << "=======================================================\n";
    std::cout << "  Working Set Size:      " << totalCount << " Thumbnails (256x256)\n";
    std::cout << "  CPU SIMD Detected:     " << simdStr << "\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << "  [BASELINE: Software JPEG Decompression]\n";
    std::cout << "    Total Decode Time:   " << std::fixed << std::setprecision(2) << comp.jpegDecodeTimeMs << " ms\n";
    std::cout << "    Avg Time Per Tile:   " << std::fixed << std::setprecision(3) << avgJpegMs << " ms\n";
    std::cout << "    L1 RAM Footprint:    " << std::fixed << std::setprecision(1) << uncompressedRamMb << " MB\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << "  [NEXT-GEN: Direct BC1 Native Texture Pipeline]\n";
    std::cout << "    Total Decode Time:   " << std::fixed << std::setprecision(2) << comp.bc1DecodeTimeMs << " ms\n";
    std::cout << "    Avg Time Per Tile:   " << std::fixed << std::setprecision(3) << avgBc1Ms << " ms\n";
    std::cout << "    BC1 RAM Footprint:   " << std::fixed << std::setprecision(1) << bc1RamMb << " MB\n";
    std::cout << "-------------------------------------------------------\n";
    std::cout << "  [COMPOUNDED OPTIMIZATION RESULTS]\n";
    std::cout << "    Memory Reduction:    " << std::fixed << std::setprecision(1) << comp.memoryReductionRatio << "x LESS RAM\n";
    std::cout << "    Throughput Speedup:  " << std::fixed << std::setprecision(1) << comp.speedupRatio << "x FASTER\n";
    std::cout << "=======================================================\n\n";

    QVariantMap resMap = json.toVariantMap();
    QMetaObject::invokeMethod(this, [this, resMap]() {
      m_results = resMap;
      m_isRunning = false;
      m_statusText = "Benchmark Completed.";
      emit resultsChanged();
      emit isRunningChanged();
      emit statusTextChanged();
      emit benchmarkCompleted(resMap);
    });
  });
}
