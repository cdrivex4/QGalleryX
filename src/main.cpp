#include <QApplication>
#include <QDateTime>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QThread>
#include <QUrl>
#include <iostream>


#include "AlbumModel.h"
#include "AsyncImageProvider.h"
#include "DesktopHelper.h"
#include "FrameBudgetScheduler.h"
#include "GroupedProxyModel.h"
#include "ImageModel.h"
#include "LogManager.h"
#include "SettingsHelper.h"
#include "SystemMonitor.h"


int main(int argc, char *argv[]) {
  // Initialize LogManager
  LogManager::instance().setLogFile("logs/application.log");
  qInstallMessageHandler(LogManager::messageHandler);

  QQuickStyle::setStyle("Basic");
  QCoreApplication::setOrganizationName("SamsungClone");
  QCoreApplication::setOrganizationDomain("samsungclone.com");
  QCoreApplication::setApplicationName("Gallery");
  QCoreApplication::setApplicationVersion("2.0.0");

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

  QApplication app(argc, argv);

  // Initialize Frame Budget Scheduler for Robustness (Prevents TDR/Main Thread
  // Flooding)
  auto frameScheduler = new FrameBudgetScheduler(&app);
  frameScheduler->setFrameBudget(
      4); // Conservative budget (4 tasks per 16ms frame)
  frameScheduler->setEnabled(true);

  // Register with AsyncImageProvider (Static)
  AsyncImageProvider::setFrameScheduler(frameScheduler);

  qmlRegisterType<ImageModel>("QGalleryX", 1, 0, "ImageModel");
  // ...
  qmlRegisterType<AlbumModel>("QGalleryX", 1, 0, "AlbumModel");
  qmlRegisterType<GroupedProxyModel>("QGalleryX", 1, 0,
                                     "GroupedProxyModel");
  qmlRegisterType<SettingsHelper>("QGalleryX", 1, 0, "SettingsHelper");
  qmlRegisterType<SystemMonitor>("QGalleryX", 1, 0, "SystemMonitor");
  qmlRegisterType<DesktopHelper>("QGalleryX", 1, 0, "DesktopHelper");

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
  DesktopHelper desktopHelper;
  engine.rootContext()->setContextProperty("desktopHelper", &desktopHelper);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  // Load Main.qml directly from resources
  const QUrl url("qrc:/QGalleryX/resources/qml/Main.qml");

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
