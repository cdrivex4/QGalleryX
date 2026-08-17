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

#include <QFile>
#include <QTextStream>

#include "LogManager.h"
#include "../src/ImageProcessor.h"
#include "../src/FileCacheManager.h"
#include "../src/PassiveReadLatencyGuard.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>

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
    qDebug() << "[main] Pre-faulted and cached 100% of executable and DLL pages into local RAM.";
}
#endif

int main(int argc, char *argv[]) {
  QCoreApplication::setOrganizationName("SamsungClone");
  QCoreApplication::setOrganizationDomain("samsungclone.com");
  QCoreApplication::setApplicationName("Gallery");
  QCoreApplication::setApplicationVersion("2.0.0");

#ifdef Q_OS_WIN
  preloadAndLockAllModules();
#endif

  // Initialize LogManager
  LogManager::instance().setLogFile("application.log");
  qInstallMessageHandler(LogManager::messageHandler);

  // Initialize Disk Cache
  FileCacheManager::instance().initialize();

  // Initialize RAM Image Cache capacity (default 512MB)
  QSettings settings("SamsungClone", "Gallery");
  int cacheSizeMB = settings.value("cacheSizeMB", 512).toInt();
  if (cacheSizeMB <= 0) cacheSizeMB = 512;
  AsyncImageProvider::setCacheMaxCost(cacheSizeMB * 1024);

  // Set style to Basic to allow customization
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

  // Increase QML's internal Image Cache to 1024MB so it can hold ~15 full 4K images
  // without evicting them constantly while swiping.
  qputenv("QML_IMAGE_CACHE_SIZE", "1024");

  // Force Qt Multimedia to use bundled FFmpeg backend for video playback
  // Unlocks native AV1, VP9, MKV, WebM, FLV, and TS playback out of the box
  qputenv("QT_MEDIA_BACKEND", "ffmpeg");

  QApplication app(argc, argv);

  // Pre-load media DLL pages (FFmpeg, LibRaw, WIC) asynchronously into RAM at startup
  // Prevents SMB page fault stalls when running over network shares
  TaskScheduler::instance().addTask([]() {
#ifdef Q_OS_WIN
      CoInitialize(NULL);
#endif
      VideoThumbnailer::warmup();
#ifdef Q_OS_WIN
      CoUninitialize();
#endif
  }, TaskScheduler::IO_BOUND, TaskScheduler::Low, "WarmupMediaDLLs");

  qmlRegisterType<ImageModel>("QGalleryX", 1, 0, "ImageModel");

  qmlRegisterType<AlbumModel>("QGalleryX", 1, 0, "AlbumModel");
  qmlRegisterType<GroupedProxyModel>("QGalleryX", 1, 0,
                                     "GroupedProxyModel");
  qmlRegisterType<SettingsHelper>("QGalleryX", 1, 0, "SettingsHelper");
  qmlRegisterType<SystemMonitor>("QGalleryX", 1, 0, "SystemMonitor");

  QQmlApplicationEngine engine;

  // Register Async Image Provider
  engine.addImageProvider("async", new AsyncImageProvider);
  AsyncImageProvider::s_logLevel.store(2); // Force Debug Log

  // Verify Image Plugins
  qDebug() << "[DIAG] Supported Image Formats:"
           << QImageReader::supportedImageFormats();

  // Expose SettingsHelper instance
  SettingsHelper settingsHelper;
  engine.rootContext()->setContextProperty("appSettings", &settingsHelper);

  // Expose SystemMonitor instance and start monitoring
  SystemMonitor systemMonitor;
  systemMonitor.startMonitoring(1000); // Update every 1 second
  engine.rootContext()->setContextProperty("systemMonitor", &systemMonitor);

  // Expose DesktopHelper
  qmlRegisterUncreatableType<DesktopHelper>("QGalleryX", 1, 0, "DesktopHelper", "Enums only");
  DesktopHelper desktopHelper;
  engine.rootContext()->setContextProperty("desktopHelper", &desktopHelper);
  engine.rootContext()->setContextProperty("viewportGovernor", &ViewportGovernor::instance());

  // Expose ImageProcessor
  ImageProcessor imageProcessor;
  engine.rootContext()->setContextProperty("imageProcessor", &imageProcessor);

  // Expose PassiveReadLatencyGuard
  engine.rootContext()->setContextProperty("latencyGuard", &PassiveReadLatencyGuard::instance());

  // Expose TaskScheduler
  engine.rootContext()->setContextProperty("taskScheduler", &TaskScheduler::instance());

  // Expose FileCacheManager
  engine.rootContext()->setContextProperty("fileCacheManager", &FileCacheManager::instance());

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  // Load Main.qml directly from resources
  const QUrl url("qrc:/QGalleryX/resources/qml_legacy/Main.qml");
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreated, &app,
      [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
          QCoreApplication::exit(-1);
      },
      Qt::QueuedConnection);
  engine.load(url);

#ifdef Q_OS_WIN
  // Process-wide UIPI bypass for Drag and Drop.
  // Avoids calling winId() before app.exec() which crashes Qt 6 RHI.
  ChangeWindowMessageFilter(WM_DROPFILES, MSGFLT_ADD);
  ChangeWindowMessageFilter(WM_COPYDATA, MSGFLT_ADD);
  ChangeWindowMessageFilter(0x0049 /* WM_COPYGLOBALDATA */, MSGFLT_ADD);
#endif

  return app.exec();
}
