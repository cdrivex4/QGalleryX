#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QSGRendererInterface>


int main(int argc, char *argv[]) {
  // Enable high DPI scaling
  QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

  // Set application information
  QCoreApplication::setOrganizationName("LMStudio");
  QCoreApplication::setOrganizationDomain("lmstudio.com");
  QCoreApplication::setApplicationName("Hello World Test");
  QCoreApplication::setApplicationVersion("1.0.0");

  QApplication app(argc, argv);

  // Set graphics API (similar to main project)
  QQuickWindow::setGraphicsApi(QSGRendererInterface::GraphicsApi::OpenGL);

  qDebug() << "Hello World Application Starting...";
  qDebug() << "Application Version:" << QCoreApplication::applicationVersion();
  qDebug() << "Qt Version:" << QT_VERSION_STR;

  QQmlApplicationEngine engine;

  // Handle object creation failures
  QObject::connect(
      &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  // Load the main QML file
  const QUrl url(u"qrc:/LMStudioTest/src/main.qml"_qs);
  engine.load(url);

  // Verify QML loaded successfully
  if (engine.rootObjects().isEmpty()) {
    qCritical() << "Failed to load QML file";
    return -1;
  }

  qDebug() << "Hello World Application Loaded Successfully";

  return app.exec();
}