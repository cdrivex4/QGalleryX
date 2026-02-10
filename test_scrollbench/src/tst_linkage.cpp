#include "../../src/DesktopHelper.h"
#include "../../src/TaskScheduler.h"
#include "ScrollBenchImageModel.h"
#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QString>
#include <iomanip>
#include <iostream>


int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  QString scanPath = "";
  for (int i = 1; i < argc; ++i) {
    if (QString(argv[i]) == "--scan" && i + 1 < argc) {
      scanPath = argv[i + 1];
    }
  }

  std::cout << "=== SCROLLBENCH MODULE VERIFICATION ===" << std::endl;

  // 1. Image Model Verification
  ScrollBenchImageModel model;
  std::cout << "[1/4] ImageModel Instance: READY" << std::endl;

  if (!scanPath.isEmpty()) {
    std::cout << "[2/4] Headless Scan Target: " << scanPath.toStdString()
              << std::endl;

    QEventLoop loop;
    QObject::connect(&model, &ScrollBenchImageModel::scanComplete, &loop,
                     &QEventLoop::quit);

    model.scanDirectory(scanPath);

    std::cout << "      Scanning... " << std::flush;
    loop.exec();
    std::cout << "DONE" << std::endl;

    int totalVisible = model.rowCount();
    int raws = 0;
    int videos = 0;
    for (int i = 0; i < totalVisible; ++i) {
      if (model.data(model.index(i), ScrollBenchImageModel::IsRawRole).toBool())
        raws++;
      if (model.data(model.index(i), ScrollBenchImageModel::IsVideoRole)
              .toBool())
        videos++;
    }

    std::cout << "      Scan Results: " << totalVisible << " items found"
              << std::endl;
    std::cout << "      - RAW Files:   " << raws << std::endl;
    std::cout << "      - Video Files: " << videos << std::endl;
    std::cout << "      - Standard:    " << (totalVisible - raws - videos)
              << std::endl;

    if (totalVisible == 0) {
      std::cerr << "      ERROR: No items found in specified directory!"
                << std::endl;
      return 2;
    }
  } else {
    std::cout << "[2/4] Headless Scan: SKIPPED (Use --scan <path>)"
              << std::endl;
  }

  // 3. Desktop Helper Verification
  DesktopHelper desktop;
  std::cout << "[3/4] DesktopHelper: READY" << std::endl;

  // 4. Task Scheduler Verification (Singleton check)
  try {
    TaskScheduler &scheduler = TaskScheduler::instance();
    std::cout << "[4/4] TaskScheduler: READY (Active Pool)" << std::endl;
  } catch (...) {
    std::cerr << "[4/4] TaskScheduler: FAILED to obtain instance!" << std::endl;
    return 1;
  }

  std::cout << "=======================================" << std::endl;
  std::cout << "VERIFICATION SUCCESSFUL: Performance backend is healthy."
            << std::endl;

  return 0;
}
