#include <QGuiApplication>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QUrl>

#include "AlbumModel.h"
#include "AsyncImageProvider.h"
#include "GroupedProxyModel.h"
#include "ImageModel.h"
#include "SettingsHelper.h"
#include "SystemMonitor.h"

int main(int argc, char *argv[]) {
  // Suppress annoying JPEG warnings
  QLoggingCategory::setFilterRules("qt.gui.imageio.jpeg.warning=false");

  // Set style to Basic to allow customization
  QQuickStyle::setStyle("Basic");

  QCoreApplication::setOrganizationName("SamsungClone");
  QCoreApplication::setOrganizationDomain("samsungclone.com");
  QCoreApplication::setApplicationName("GalleryTest");

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

  QGuiApplication app(argc, argv);

  qmlRegisterType<ImageModel>("SamsungGalleryTest", 1, 0, "ImageModel");
  qmlRegisterType<AlbumModel>("SamsungGalleryTest", 1, 0, "AlbumModel");
  qmlRegisterType<GroupedProxyModel>("SamsungGalleryTest", 1, 0,
                                     "GroupedProxyModel");
  qmlRegisterType<SettingsHelper>("SamsungGalleryTest", 1, 0, "SettingsHelper");
  qmlRegisterType<SystemMonitor>("SamsungGalleryTest", 1, 0, "SystemMonitor");

  QQmlApplicationEngine engine;

  // Register Async Image Provider
  engine.addImageProvider("async", new AsyncImageProvider);

  // Expose SettingsHelper instance
  SettingsHelper settingsHelper;
  engine.rootContext()->setContextProperty("appSettings", &settingsHelper);

  // Expose SystemMonitor instance and start monitoring
  SystemMonitor systemMonitor;
  systemMonitor.startMonitoring(1000); // Update every 1 second
  engine.rootContext()->setContextProperty("systemMonitor", &systemMonitor);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  // Load MainSemantic.qml directly from resources
  const QUrl url("qrc:/SamsungGalleryTest/resources/qml/MainSemantic.qml");
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
