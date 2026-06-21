#include "AlbumModel.h"
#include "AsyncImageProvider.h"
#include "DesktopHelper.h"
#include "DiagnosticsMonitor.h"
#include "FrameBudgetScheduler.h"
#include "GroupedProxyModel.h"
#include "HardwareAccelerationManager.h"
#include "ImageProcessor.h"
#include "ScrollBenchImageModel.h"
#include "SettingsHelper.h"
#include "SystemMonitor.h"
#include "TaskScheduler.h"
#include "TelemetryMonitor.h"
#include "FastImageItem.h"
#include <QDateTime>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QString>
#include <QTextStream>
#include <QTimer>
#include <QtGlobal>
#include <csignal>
#include <cstdio>

#ifdef Q_OS_WIN
#include <windows.h>

LONG WINAPI GlobalExceptionHandler(EXCEPTION_POINTERS *ExceptionInfo) {
  QString errorMsg =
      QString("CRITICAL CRASH: Exception Code 0x%1")
          .arg(ExceptionInfo->ExceptionRecord->ExceptionCode, 0, 16);
  MessageBoxA(NULL, errorMsg.toUtf8().constData(),
              "ScrollBench - Crash Detected", MB_ICONERROR | MB_OK);
  return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void signalHandler(int signum) {
  QString msg =
      QString("FATAL SIGNAL %1 (SIGSEGV/SIGABRT) received.").arg(signum);
#ifdef Q_OS_WIN
  MessageBoxA(NULL, msg.toUtf8().constData(), "ScrollBench - Crash Detected",
              MB_ICONERROR | MB_OK);
#endif
  qFatal("%s", msg.toUtf8().constData());
}

void crashHandler(QtMsgType type, const QMessageLogContext &context,
                  const QString &msg) {
  QTextStream ts(stderr);
  ts << msg << "\n";

  // Force flush relevant performance logs to disk
  if (type == QtFatalMsg || type == QtCriticalMsg ||
      msg.contains("[AdaptiveIO]") || msg.contains("[Performance]")) {
    QDir().mkpath("logs");
    FILE *f = fopen("logs/crash.log", "a");
    if (f) {
      fprintf(f, "%s\n", msg.toUtf8().constData());
      fflush(f);
      fclose(f);
    }
  }
}

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
  SetUnhandledExceptionFilter(GlobalExceptionHandler);
#endif
  std::signal(SIGSEGV, signalHandler);
  std::signal(SIGABRT, signalHandler);

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
  qmlRegisterType<FastImageItem>("ScrollBenchBackend", 1, 0, "FastImage");

  // Register async image provider for real images
  engine.addImageProvider(QLatin1String("async"), new AsyncImageProvider());
  AsyncImageProvider::s_logLevel.store(2); // MAXIMUM DEBUG LOGGING

  // Create core components on heap for safer destruction
  auto *imageModel = new ScrollBenchImageModel();
  auto *frameBudget = new FrameBudgetScheduler();
  auto *systemMonitor = new SystemMonitor();
  auto *telemetry = new TelemetryMonitor();
  auto *settings = new SettingsHelper();
  auto *desktopHelper = new DesktopHelper();
  auto *albumModel = new AlbumModel();
  auto *imageProcessor = new ImageProcessor(); // Create ImageProcessor instance
  auto *diagnostics = new DiagnosticsMonitor(); // Create diagnostics monitor
  systemMonitor->startMonitoring(1000);

  // Connect model and frame budget
  imageModel->setFrameScheduler(frameBudget);

  // Attach diagnostics to components for monitoring
  diagnostics->attachModel(imageModel);
  diagnostics->attachSettings(settings);

  // Generate 10,000 test items by default
  // imageModel->generateTestData(10000); // Disabled for clean performance
  // monitoring

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
  engine.rootContext()->setContextProperty("diagnostics",
                                           diagnostics); // Expose diagnostics
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

  // Handle auto-scan if path provided in CLI arguments
  QStringList args = QCoreApplication::arguments();
  if (args.count() >= 3 && args.at(1) == "--scan") {
    QString scanPath = args.at(2);
    qDebug() << "[AUTO-SCAN] Triggering for:" << scanPath;
    imageModel->scanDirectory(scanPath);
  }

  return app.exec();
}
