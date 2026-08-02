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

#include "AlbumModel.h"
#include "AsyncImageProvider.h"
#include "DesktopHelper.h"
#include "GroupedProxyModel.h"
#include "ImageModel.h"
#include "SettingsHelper.h"
#include "SystemMonitor.h"
#include "TaskScheduler.h"

#include <QFile>
#include <QTextStream>

#include "LogManager.h"
#include "../src/ImageProcessor.h"
#include "../src/FileCacheManager.h"
#include "../src/PassiveReadLatencyGuard.h"

// Remove customMessageHandler function completely

int main(int argc, char *argv[]) {
  // Initialize LogManager
  LogManager::instance().setLogFile("application.log");
  qInstallMessageHandler(LogManager::messageHandler);

  // Initialize Disk Cache
  FileCacheManager::instance().initialize();

  // Suppress annoying JPEG warnings (Commented out to restore granularity)
  // QLoggingCategory::setFilterRules("qt.gui.imageio.jpeg.warning=false");

  // Set style to Basic to allow customization
  QQuickStyle::setStyle("Basic");

  QCoreApplication::setOrganizationName("SamsungClone");
  QCoreApplication::setOrganizationDomain("samsungclone.com");
  QCoreApplication::setApplicationName("Gallery");
  QCoreApplication::setApplicationVersion("2.0.0");

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

  QApplication app(argc, argv);

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

  // Expose ImageProcessor
  ImageProcessor imageProcessor;
  engine.rootContext()->setContextProperty("imageProcessor", &imageProcessor);

  // Expose PassiveReadLatencyGuard
  engine.rootContext()->setContextProperty("latencyGuard", &PassiveReadLatencyGuard::instance());

  // Expose TaskScheduler
  engine.rootContext()->setContextProperty("taskScheduler", &TaskScheduler::instance());

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

  return app.exec();
}
