#include <QApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QUrl>

#include <QDateTime>
#include <QThread>
#include <iostream>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

#include "AlbumModel.h"
#include "AsyncImageProvider.h"
#include "DesktopHelper.h"
#include "GroupedProxyModel.h"
#include "ImageModel.h"
#include "SettingsHelper.h"
#include "SystemMonitor.h"
#include "TaskScheduler.h"
#include "VideoThumbnailer.h"
#include "ViewportGovernor.h"
#include "BenchmarkRunner.h"
#include "BC1Engine.h"

#include <QFile>
#include <QTextStream>

#include "LogManager.h"
#include "ImageProcessor.h"
#include "FileCacheManager.h"
#include "PassiveReadLatencyGuard.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <dbghelp.h>

static LONG WINAPI customCrashFilter(EXCEPTION_POINTERS *pExceptionPointers) {
    DWORD code = pExceptionPointers ? pExceptionPointers->ExceptionRecord->ExceptionCode : 0;
    void *addr = pExceptionPointers ? pExceptionPointers->ExceptionRecord->ExceptionAddress : nullptr;

    FILE *f = fopen("scrollbench_crash.log", "a");
    if (f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] FATAL EXCEPTION: Code 0x%08lX at Address %p\n",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                code, addr);
        fflush(f);
        fclose(f);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

static void preloadAndLockAllModules() {
    HANDLE hProcess = GetCurrentProcess();
    HMODULE hMods[1024];
    DWORD cbNeeded = 0;
    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        DWORD count = cbNeeded / sizeof(HMODULE);
        for (DWORD i = 0; i < count; i++) {
            MODULEINFO modInfo;
            if (GetModuleInformation(hProcess, hMods[i], &modInfo, sizeof(modInfo))) {
                volatile const char *base = (volatile const char *)modInfo.lpBaseOfDll;
                size_t size = modInfo.SizeOfImage;
                for (size_t offset = 0; offset < size; offset += 4096) {
                    char dummy = base[offset];
                    Q_UNUSED(dummy);
                }
            }
        }
    }
    qDebug() << "[main_scrollbench] Pre-faulted and cached 100% of executable and DLL pages into local RAM.";
}
#endif

#include "BuildInfo.h"

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
  SetUnhandledExceptionFilter(customCrashFilter);
#endif
  std::set_terminate([]() {
    qCritical() << "[FATAL] std::terminate called in QGalleryXBench!";
    abort();
  });

  // 1. Output version banner and build details
  printf("====================================================\n");
  printf("  QGalleryXBench v%s (Build %d)\n", BUILD_VERSION, BUILD_NUMBER);
  printf("  Derived from QGalleryX | SIMD: %s\n", BC1Engine::simdLevelString(BC1Engine::detectSimdLevel()));
  printf("  Built: %s | Mode: Dynamic Release (Qt 6.9.3)\n", BUILD_TIMESTAMP);
  printf("====================================================\n\n");
  fflush(stdout);

  QCoreApplication::setOrganizationName("SamsungClone");
  QCoreApplication::setOrganizationDomain("samsungclone.com");
  QCoreApplication::setApplicationName("GalleryBench");
  QCoreApplication::setApplicationVersion(BUILD_VERSION);

  // Set style to Basic to match QGalleryX
  QQuickStyle::setStyle("Basic");

  // Set up graphics API before creating QGuiApplication
  SettingsHelper tempHelper;
  int api = tempHelper.selectedApi();

  if (api == 1)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
  else if (api == 2)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);
  else if (api == 3)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
  else if (api == 4)
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);

  qputenv("QML_IMAGE_CACHE_SIZE", "1024");
  qputenv("QT_MEDIA_BACKEND", "ffmpeg");

  QApplication app(argc, argv);

#ifdef Q_OS_WIN
  preloadAndLockAllModules();
#endif

  LogManager::instance().setLogFile("scrollbench.log");
  qInstallMessageHandler(LogManager::messageHandler);

  FileCacheManager::instance().initialize();

  QSettings settings("SamsungClone", "Gallery");
  int cacheSizeMB = settings.value("cacheSizeMB", 512).toInt();
  if (cacheSizeMB <= 0) cacheSizeMB = 512;
  AsyncImageProvider::setCacheMaxCost(cacheSizeMB * 1024);

  TaskScheduler::instance().addTask([]() {
#ifdef Q_OS_WIN
      CoInitialize(NULL);
#endif
      VideoThumbnailer::warmup();
#ifdef Q_OS_WIN
      CoUninitialize();
#endif
  }, TaskScheduler::IO_BOUND, TaskScheduler::Low, "WarmupMediaDLLs");

  // Register all QGalleryX Types
  qmlRegisterType<ImageModel>("QGalleryX", 1, 0, "ImageModel");
  qmlRegisterType<AlbumModel>("QGalleryX", 1, 0, "AlbumModel");
  qmlRegisterType<GroupedProxyModel>("QGalleryX", 1, 0, "GroupedProxyModel");
  qmlRegisterType<SettingsHelper>("QGalleryX", 1, 0, "SettingsHelper");
  qmlRegisterType<SystemMonitor>("QGalleryX", 1, 0, "SystemMonitor");
  qmlRegisterType<BenchmarkRunner>("QGalleryX", 1, 0, "BenchmarkRunner");

  QQmlApplicationEngine engine;

  engine.addImageProvider("async", new AsyncImageProvider);
  AsyncImageProvider::s_logLevel.store(2);

  SettingsHelper settingsHelper;
  engine.rootContext()->setContextProperty("appSettings", &settingsHelper);

  SystemMonitor systemMonitor;
  systemMonitor.startMonitoring(1000);
  engine.rootContext()->setContextProperty("systemMonitor", &systemMonitor);

  qmlRegisterUncreatableType<DesktopHelper>("QGalleryX", 1, 0, "DesktopHelper", "Enums only");
  DesktopHelper desktopHelper;
  DesktopHelper::setEngine(&engine);
  engine.rootContext()->setContextProperty("desktopHelper", &desktopHelper);
  engine.rootContext()->setContextProperty("viewportGovernor", &ViewportGovernor::instance());

  ImageProcessor imageProcessor;
  engine.rootContext()->setContextProperty("imageProcessor", &imageProcessor);

  engine.rootContext()->setContextProperty("latencyGuard", &PassiveReadLatencyGuard::instance());
  engine.rootContext()->setContextProperty("taskScheduler", &TaskScheduler::instance());
  engine.rootContext()->setContextProperty("fileCacheManager", &FileCacheManager::instance());

  BenchmarkRunner benchmarkRunner;
  engine.rootContext()->setContextProperty("benchmarkRunner", &benchmarkRunner);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  const QUrl url("qrc:/ScrollBench/qml/Main.qml");
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
          QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);
  engine.load(url);

#ifdef Q_OS_WIN
  ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
  ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
  ChangeWindowMessageFilter(0x0049, MSGFLT_ADD);
#endif

  // Handle automated CLI benchmark
  QStringList args = QCoreApplication::arguments();
  if (args.contains("--benchmark")) {
    QString benchPath = "C:/";
    int targetSet = 4840;
    int pathIdx = args.indexOf("--path");
    if (pathIdx != -1 && pathIdx + 1 < args.size()) {
      benchPath = args[pathIdx + 1];
    }
    int setIdx = args.indexOf("--target-set");
    if (setIdx != -1 && setIdx + 1 < args.size()) {
      targetSet = args[setIdx + 1].toInt();
    }
    qDebug() << "[BENCHMARK] Automated drive benchmark scheduled on" << benchPath;
    QTimer::singleShot(500, [&benchmarkRunner, benchPath, targetSet]() {
      benchmarkRunner.runDriveBenchmark(benchPath, targetSet);
    });
  }

  int ret = app.exec();
  TaskScheduler::instance().stop();
  return ret;
}
