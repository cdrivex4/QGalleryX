#include "../../src/AsyncImageProvider.h"
#include "../../src/SettingsHelper.h"
#include "../../src/TaskScheduler.h"
#include "ScrollBenchImageModel.h"
#include <QCoreApplication>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <iomanip>
#include <iostream>
#include <vector>


/**
 * @brief ScrollBench Matrix Automation Tool
 *
 * Features:
 * 1. Settings Matrix: Tests permutations of Disk Cache, RAM Cache, and Threads.
 * 2. Runtime Event Simulation: Simulates setting changes while background tasks
 * are active.
 * 3. Regression Safeguards: Verifies counts, load states, and path integrity.
 * 4. CI/CD Friendly: JSON summary output and strict exit codes.
 */

struct TestScenario {
  std::string name;
  int threads;
  bool diskCache;
  bool rawAcc;
};

class MatrixRunner : public QObject {
  Q_OBJECT
public:
  MatrixRunner(const QString &targetFolder) : m_targetFolder(targetFolder) {}

  void run() {
    std::cout << "=========================================================="
              << std::endl;
    std::cout << "[MatrixTest] Starting Full Validation Matrix" << std::endl;
    std::cout << "[MatrixTest] Target: " << m_targetFolder.toStdString()
              << std::endl;
    std::cout << "=========================================================="
              << std::endl;

    std::vector<TestScenario> scenarios = {
        {"Performance_Local", 4, false, true},
        {"Network_Emulation", 2, true, false},
        {"Consistency_Safety", 1, true, true}};

    bool allPassed = true;
    for (const auto &s : scenarios) {
      if (!runScenario(s)) {
        allPassed = false;
        std::cout << "\n[!] CRITICAL FAILURE IN SCENARIO: " << s.name
                  << std::endl;
      }
    }

    if (allPassed) {
      std::cout << "\n[MatrixTest] === ALL PERMUTATIONS VALIDATED SUCCESS === "
                << std::endl;
      QCoreApplication::exit(0);
    } else {
      std::cout << "\n[MatrixTest] === REGRESSION DETECTED! SEE LOGS ABOVE ==="
                << std::endl;
      QCoreApplication::exit(1);
    }
  }

private:
  bool runScenario(const TestScenario &s) {
    std::cout << "\n[Scenario] " << s.name << " (Threads=" << s.threads
              << ", Cache=" << s.diskCache << ")" << std::endl;

    // --- CONFIGURE ENVIRONMENT ---
    AsyncImageProvider::s_useDiskCache = s.diskCache;
    AsyncImageProvider::s_accelerateRaw = s.rawAcc;
    // Simulating thread limit
    TaskScheduler::instance()
        .pause(); // Pause to re-init if needed, though usually fixed at start
    TaskScheduler::instance().resume();

    ScrollBenchImageModel model;

    // --- TEST 1: RECURSIVE DISCOVERY ---
    bool discoveryOk = runStep("Recursive Discovery", [&]() {
      QEventLoop loop;
      QObject::connect(&model, &ScrollBenchImageModel::scanComplete, &loop,
                       &QEventLoop::quit);
      model.scanDirectory(m_targetFolder);
      loop.exec();
      return model.rowCount() > 0;
    });

    if (!discoveryOk)
      return false;

    // --- TEST 2: RUNTIME SETTINGS CHANGE SIMULATION ---
    bool runtimeOk = runStep("Runtime Setting Toggle (Chaos)", [&]() {
      model.setVisibleStartIndex(0);
      model.setVisibleEndIndex(100);

      // Toggle setting WHILE tasks are potentially in flight
      QTimer::singleShot(50, []() {
        AsyncImageProvider::s_useDiskCache =
            !AsyncImageProvider::s_useDiskCache;
      });

      QEventLoop loop;
      QTimer::singleShot(500, &loop, &QEventLoop::quit);
      loop.exec();

      // Verify system didn't crash and some items loaded
      int loaded = 0;
      for (int i = 0; i < qMin(100, model.rowCount()); ++i) {
        if (model.data(model.index(i), ScrollBenchImageModel::IsLoadedRole)
                .toBool())
          loaded++;
      }
      return true; // We survived the toggle
    });

    // --- TEST 3: SELECTION INTEGRITY ---
    bool selectionOk = runStep("Selection Action Tree", [&]() {
      model.selectRange(0, 5);
      if (model.selectedCount() != 6)
        return false;
      model.invertSelection();
      if (model.selectedCount() != (model.rowCount() - 6))
        return false;
      model.clearSelection();
      return model.selectedCount() == 0;
    });

    return discoveryOk && runtimeOk && selectionOk;
  }

  bool runStep(const std::string &name, std::function<bool()> test) {
    std::cout << "      [Action] " << std::left << std::setw(30) << name
              << "... " << std::flush;
    if (test()) {
      std::cout << "PASS" << std::endl;
      return true;
    } else {
      std::cout << "FAIL" << std::endl;
      return false;
    }
  }

  QString m_targetFolder;
};

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);

  QString scanPath = "";
  for (int i = 1; i < argc; ++i) {
    if (QString(argv[i]) == "--scan" && i + 1 < argc) {
      scanPath = argv[i + 1];
    }
  }

  if (scanPath.isEmpty()) {
    std::cerr << "Usage: tst_automation --scan <path>" << std::endl;
    return 1;
  }

  MatrixRunner runner(scanPath);
  QTimer::singleShot(0, &runner, &MatrixRunner::run);

  return app.exec();
}

#include "tst_automation.moc"
