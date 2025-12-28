#include "../src/FastVolumeScanner.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDirIterator>
#include <QElapsedTimer>
#include <iostream>


int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  std::cout << "=== MFT Scanner Benchmark ===" << std::endl;

  // 1. Benchmark FastScanner
  std::cout << "[1] FastVolumeScanner (MFT)" << std::endl;
  FastVolumeScanner scanner;
  QElapsedTimer timer;
  timer.start();

  // Assumption: Scanning C: drive
  QString scanPath = "C:/";

  if (scanner.scanVolume(scanPath)) {
    qint64 elapsed = timer.elapsed();
    QVector<QString> files = scanner.getAllFiles();
    QVector<QString> dirs = scanner.getAllDirectories();

    std::cout << "    Success!" << std::endl;
    std::cout << "    Time: " << elapsed << " ms" << std::endl;
    std::cout << "    Files Found: " << files.size() << std::endl;
    std::cout << "    Dirs Found: " << dirs.size() << std::endl;
  } else {
    std::cerr << "    FAILED (Check Admin Rights)" << std::endl;
  }

  // 2. Benchmark Standard Iterator (Limited)
  std::cout << "\n[2] QDirIterator (Standard - Limit 1000)" << std::endl;
  timer.restart();
  QDirIterator it(scanPath, QDir::Files | QDir::NoDotAndDotDot,
                  QDirIterator::Subdirectories);

  int count = 0;
  while (it.hasNext() && count < 1000) {
    it.next();
    count++;
  }

  std::cout << "    Scanned 1000 files in: " << timer.elapsed() << " ms"
            << std::endl;

  return 0;
}
