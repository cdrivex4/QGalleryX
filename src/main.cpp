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

#include <QFile>
#include <QTextStream>

void customMessageHandler(QtMsgType type, const QMessageLogContext &context,
                          const QString &msg) {
  QString txt;
  QString timeStr =
      QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
  QString threadId = QString::number((quint64)QThread::currentThreadId());

  switch (type) {
  case QtDebugMsg:
    txt = QString("[%1] [Thread %2] Debug: %3").arg(timeStr, threadId, msg);
    break;
  case QtWarningMsg:
    txt = QString("[%1] [Thread %2] Warning: %3").arg(timeStr, threadId, msg);
    break;
  case QtCriticalMsg:
    txt = QString("[%1] [Thread %2] Critical: %3").arg(timeStr, threadId, msg);
    break;
  case QtFatalMsg:
    txt = QString("[%1] [Thread %2] Fatal: %3").arg(timeStr, threadId, msg);
    break;
  case QtInfoMsg:
    txt = QString("[%1] [Thread %2] Info: %3").arg(timeStr, threadId, msg);
    break;
  }

  // Output to std::cout/err for immediate visibility in console/redirect
  if (type == QtCriticalMsg || type == QtFatalMsg) {
    std::cerr << txt.toStdString() << std::endl;
  } else {
    std::cout << txt.toStdString() << std::endl;
  }

  // Output to file
  QFile outFile("application.log");
  if (outFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
    QTextStream ts(&outFile);
    ts << txt << Qt::endl;
  }
}

int main(int argc, char *argv[]) {
  qInstallMessageHandler(customMessageHandler);

  // Suppress annoying JPEG warnings
  QLoggingCategory::setFilterRules("qt.gui.imageio.jpeg.warning=false");

  // Set style to Basic to allow customization
  QQuickStyle::setStyle("Basic");

  QCoreApplication::setOrganizationName("SamsungClone");
  QCoreApplication::setOrganizationDomain("samsungclone.com");
  QCoreApplication::setApplicationName("Gallery");

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

  QApplication app(argc, argv);

  qmlRegisterType<ImageModel>("SamsungGallery", 1, 0, "ImageModel");
  qmlRegisterType<AlbumModel>("SamsungGallery", 1, 0, "AlbumModel");
  qmlRegisterType<GroupedProxyModel>("SamsungGallery", 1, 0,
                                     "GroupedProxyModel");
  qmlRegisterType<SettingsHelper>("SamsungGallery", 1, 0, "SettingsHelper");
  qmlRegisterType<SystemMonitor>("SamsungGallery", 1, 0, "SystemMonitor");

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

  // Expose DesktopHelper
  DesktopHelper desktopHelper;
  engine.rootContext()->setContextProperty("desktopHelper", &desktopHelper);

  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  // Load Main.qml directly from resources
  const QUrl url("qrc:/SamsungGallery/resources/qml/Main.qml");
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
