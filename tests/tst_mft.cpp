#include "../src/FastVolumeScanner.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDirIterator>
#include <QElapsedTimer>
#include <iostream>


int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  QString scanPath = (argc > 1) ? QString::fromLocal8Bit(argv[1]) : "D:/";
  std::cout << "==========================================" << std::endl;
  std::cout << "=== MFT Scanner vs QDirIterator Benchmark ===" << std::endl;
  std::cout << "Target Path: " << scanPath.toStdString() << std::endl;
  std::cout << "==========================================" << std::endl;

  // 1. Benchmark FastScanner (Direct MFT)
  std::cout << "\n[1] FastVolumeScanner (Direct NTFS $MFT Streaming)" << std::endl;
  FastVolumeScanner scanner;
  QElapsedTimer timer;
  timer.start();

  qint64 mftElapsed = 0;
  int mftFiles = 0;
  if (scanner.scanVolume(scanPath)) {
    mftElapsed = timer.elapsed();
    QVector<ScannedFile> files = scanner.getScannedFiles();
    mftFiles = files.size();

    double throughput = mftElapsed > 0 ? (mftFiles * 1000.0 / mftElapsed) : 0.0;
    std::cout << "    Status: SUCCESS" << std::endl;
    std::cout << "    Time Elapsed: " << mftElapsed << " ms" << std::endl;
    std::cout << "    Total Files Found: " << mftFiles << std::endl;
    std::cout << "    Throughput: " << (long)throughput << " files/sec" << std::endl;
  } else {
    std::cerr << "    Status: FAILED (Requires Admin Rights for direct raw sector read \\\\.\\"
              << scanPath.left(2).toStdString() << ")" << std::endl;
  }

  // 2. Benchmark QDirIterator (Standard Recursive Win32 API)
  std::cout << "\n[2] QDirIterator (Standard Recursive Win32 FindFirstFileEx)" << std::endl;
  timer.restart();
  QDirIterator it(scanPath, QDir::Files | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);

  int iterCount = 0;
  int maxLimit = 50000;
  while (it.hasNext() && iterCount < maxLimit) {
    it.next();
    iterCount++;
  }
  qint64 iterElapsed = timer.elapsed();
  double iterThroughput = iterElapsed > 0 ? (iterCount * 1000.0 / iterElapsed) : 0.0;

  std::cout << "    Status: COMPLETED" << std::endl;
  std::cout << "    Time Elapsed: " << iterElapsed << " ms" << std::endl;
  std::cout << "    Files Scanned: " << iterCount << (it.hasNext() ? " (Sample capped at 50,000)" : " (Full scan)") << std::endl;
  std::cout << "    Throughput: " << (long)iterThroughput << " files/sec" << std::endl;

  // 3. Comparison
  std::cout << "\n==========================================" << std::endl;
  if (mftElapsed > 0 && iterThroughput > 0) {
    double mftThroughput = (mftFiles * 1000.0 / mftElapsed);
    double speedup = mftThroughput / iterThroughput;
    std::cout << ">>> Direct $MFT is " << speedup << "x FASTER than QDirIterator <<<" << std::endl;
  }
  std::cout << "==========================================" << std::endl;

  return 0;
}
