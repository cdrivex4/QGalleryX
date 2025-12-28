#include "AlbumModel.h"
#include "AsyncImageProvider.h"
#include "DesktopHelper.h"
#include "FrameBudgetScheduler.h"
#include "GroupedProxyModel.h"
#include "HardwareAccelerationManager.h"
#include "ImageProcessor.h" // Include ImageProcessor header
#include "ScrollBenchImageModel.h"
#include "SettingsHelper.h"
#include "SystemMonitor.h"
#include "TaskScheduler.h"
#include "TelemetryMonitor.h"
#include <QDateTime> // Added for QDateTime
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTextStream>
#include <QTimer>
#include <QtGlobal>

#include <cstdio>

void crashHandler(QtMsgType type, const QMessageLogContext &,
                  const QString &msg) {
  QTextStream ts(stderr);
  ts << msg << "\n";
  QDir().mkpath("logs");
  FILE *f = fopen("logs/crash.log", "a");
  if (f) {
    fprintf(f, "%s\n", msg.toUtf8().constData());
    fflush(f);
    fclose(f);
  }
}

int main(int argc, char *argv[]) {
  qInstallMessageHandler(crashHandler);
  // Clear previous log
  QDir().mkpath("logs");
  FILE *f = fopen("logs/crash.log", "w");
  if (f)
    fclose(f);

  QGuiApplication app(argc, argv);

  QQuickStyle::setStyle("Basic");
  QQmlApplicationEngine engine;

  // Register types for QML
  qmlRegisterType<GroupedProxyModel>("ScrollBenchBackend", 1, 0,
                                     "GroupedProxyModel");
  qmlRegisterType<ScrollBenchImageModel>("ScrollBenchBackend", 1, 0,
                                         "ScrollBenchImageModel");

  // Register async image provider for real images
  engine.addImageProvider(QLatin1String("async"), new AsyncImageProvider());

  // Create core components on heap for safer destruction
  auto *imageModel = new ScrollBenchImageModel();
  auto *frameBudget = new FrameBudgetScheduler();
  auto *telemetry = new TelemetryMonitor();
  auto *settings = new SettingsHelper();
  auto *desktopHelper = new DesktopHelper();
  auto *albumModel = new AlbumModel();
  auto *systemMonitor = new SystemMonitor();
  auto *imageProcessor = new ImageProcessor(); // Create ImageProcessor instance
  systemMonitor->startMonitoring(1000);

  // Connect model and frame budget
  imageModel->setFrameScheduler(frameBudget);

  // Generate 10,000 test items by default
  imageModel->generateTestData(10000);

  // Connect model to telemetry
  QObject::connect(
      imageModel, &ScrollBenchImageModel::pendingDecodeCountChanged, telemetry,
      [telemetry, imageModel]() {
        telemetry->setPendingDecodes(imageModel->pendingDecodeCount());
      });

  // Connect FrameBudget completions to Telemetry
  QObject::connect(
      frameBudget, &FrameBudgetScheduler::completionsThisFrameChanged,
      telemetry, [telemetry, frameBudget]() {
        telemetry->setCompletionsThisFrame(frameBudget->completionsThisFrame());
      });

  // Expose to QML
  engine.rootContext()->setContextProperty("imageModel", imageModel);
  engine.rootContext()->setContextProperty("frameBudget", frameBudget);
  engine.rootContext()->setContextProperty("telemetry", telemetry);
  engine.rootContext()->setContextProperty("settings", settings);
  engine.rootContext()->setContextProperty("desktopHelper", desktopHelper);
  engine.rootContext()->setContextProperty("albumModel", albumModel);
  engine.rootContext()->setContextProperty("systemMonitor", systemMonitor);
  engine.rootContext()->setContextProperty("taskScheduler",
                                           &TaskScheduler::instance());
  engine.rootContext()->setContextProperty(
      "hwAccel", &HardwareAccelerationManager::instance());
  engine.rootContext()->setContextProperty("imageProcessor",
                                           imageProcessor); // Expose to QML

  // Initialize Telemetry
  telemetry->setPendingDecodes(imageModel->pendingDecodeCount());
  telemetry->updateMemoryUsage();

  // Load QML
  const QUrl url(QStringLiteral("qrc:/ScrollBench/qml/MainScrollBench.qml"));

  // Exit if QML fails to load
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
          QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);

  engine.load(url);

  // Ensure app exits when all windows are closed (fixes orphaned background
  // threads)
  app.setQuitOnLastWindowClosed(true);

  return app.exec();
}
