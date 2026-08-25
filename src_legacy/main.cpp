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
#include "NativeMediaPlayer.h"

#include <QFile>
#include <QTextStream>

#include "LogManager.h"
#include "../src/ImageProcessor.h"
#include "../src/FileCacheManager.h"
#include "../src/PassiveReadLatencyGuard.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#include <dbghelp.h>

static LONG WINAPI customCrashFilter(EXCEPTION_POINTERS *pExceptionPointers) {
    DWORD code = pExceptionPointers ? pExceptionPointers->ExceptionRecord->ExceptionCode : 0;
    void *addr = pExceptionPointers ? pExceptionPointers->ExceptionRecord->ExceptionAddress : nullptr;

    FILE *f = fopen("application_crash.log", "a");
    if (f) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(f, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] FATAL EXCEPTION: Code 0x%08lX at Address %p\n",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                code, addr);
        fflush(f);
        fclose(f);
    }

    HMODULE hDbgHelp = LoadLibraryA("dbghelp.dll");
    if (hDbgHelp) {
        typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(
            HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
            PMINIDUMP_EXCEPTION_INFORMATION,
            PMINIDUMP_USER_STREAM_INFORMATION,
            PMINIDUMP_CALLBACK_INFORMATION
        );
        MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
        if (pDump) {
            HANDLE hFile = CreateFileA("crash_dump.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hFile != INVALID_HANDLE_VALUE) {
                MINIDUMP_EXCEPTION_INFORMATION mei;
                mei.ThreadId = GetCurrentThreadId();
                mei.ExceptionPointers = pExceptionPointers;
                mei.ClientPointers = FALSE;
                pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, NULL, NULL);
                CloseHandle(hFile);
            }
        }
        FreeLibrary(hDbgHelp);
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
    qDebug() << "[main] Pre-faulted and cached 100% of executable and DLL pages into local RAM.";
}
#endif

#include "BuildInfo.h"

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
  SetUnhandledExceptionFilter(customCrashFilter);
#endif
  std::set_terminate([]() {
    qCritical() << "[FATAL] std::terminate called!";
    abort();
  });
  // 1. Output version banner and build details as the very first message
  printf("====================================================\n");
  printf("  QGalleryX v%s (Build %d)\n", BUILD_VERSION, BUILD_NUMBER);
  printf("  Built: %s | Mode: Dynamic Release (Qt 6.9.3)\n", BUILD_TIMESTAMP);
  printf("====================================================\n\n");
  fflush(stdout);

  QCoreApplication::setOrganizationName("SamsungClone");
  QCoreApplication::setOrganizationDomain("samsungclone.com");
  QCoreApplication::setApplicationName("Gallery");
  QCoreApplication::setApplicationVersion(BUILD_VERSION);

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
  // Disable D3D11 hwaccel in Qt FFmpeg plugin so GPUs lacking native AV1 ASICs
  // seamlessly decode via high-performance libdav1d without D3D11 setup failure errors
  qputenv("QT_FFMPEG_DECODING_HW_DEVICE_TYPES", "none");

  // Create QApplication BEFORE initializing managers that access arguments / event loops
  QApplication app(argc, argv);

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
  qmlRegisterType<NativeMediaPlayer>("QGalleryX", 1, 0, "NativeMediaPlayer");

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
  DesktopHelper::setEngine(&engine);
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

  int result = app.exec();
  TaskScheduler::instance().stop();
  return result;
}
