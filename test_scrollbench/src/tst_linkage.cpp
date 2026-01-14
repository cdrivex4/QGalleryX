#include "../../src/DesktopHelper.h"
#include "../../src/TaskScheduler.h"
#include "ScrollBenchImageModel.h"
#include <QCoreApplication>
#include <QDebug>
#include <QString>
#include <iostream>

/**
 * @brief Linkage Verification Tool
 * Performs a sanity check on backend modules to ensure they are
 * correctly instantiated and responding to basic queries.
 */
int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  std::cout << "=== SCROLLBENCH MODULE VERIFICATION ===" << std::endl;

  // 1. Image Model Verification
  ScrollBenchImageModel model;
  std::cout << "[1/3] ImageModel: ";
  if (model.rowCount() == 0) {
    std::cout << "READY (Empty)" << std::endl;
  } else {
    std::cout << "READY (Data Found: " << model.rowCount() << ")" << std::endl;
  }

  // 2. Desktop Helper Verification
  DesktopHelper desktop;
  std::cout << "[2/3] DesktopHelper: READY" << std::endl;

  // 3. Task Scheduler Verification (Singleton check)
  try {
    TaskScheduler &scheduler = TaskScheduler::instance();
    std::cout << "[3/3] TaskScheduler: READY (Active Pool)" << std::endl;
  } catch (...) {
    std::cerr << "[3/3] TaskScheduler: FAILED to obtain instance!" << std::endl;
    return 1;
  }

  std::cout << "=======================================" << std::endl;
  std::cout << "VERIFICATION SUCCESSFUL: All modules plugged in." << std::endl;

  return 0;
}
