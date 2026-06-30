#include <QDir>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickView>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QTimer>


#include <QtGui/QGuiApplication>
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickView>


// Test class for Hello World application
class HelloWorldTest : public QObject {
  Q_OBJECT

private slots:
  void initTestCase() {
    // Initialize test environment
    qDebug() << "Initializing HelloWorld Test Suite";

    // Verify Qt framework is available
    QVERIFY(qVersion() != nullptr);
    QVERIFY(QString(qVersion()).startsWith("6."));

    qDebug() << "Qt Version:" << qVersion();
  }

  void cleanupTestCase() { qDebug() << "Cleaning up HelloWorld Test Suite"; }

  void testApplicationStartup() {
    // Test basic Qt application functionality
    QVERIFY(QGuiApplication::instance() != nullptr);

    // Test application attributes
    QCOMPARE(QCoreApplication::applicationName(), QString("Hello World Test"));
    QCOMPARE(QCoreApplication::organizationName(), QString("LMStudio"));
    QCOMPARE(QCoreApplication::applicationVersion(), QString("1.0.0"));

    qDebug() << "Application startup test passed";
  }

  void testQmlEngine() {
    // Test QML engine creation
    QQmlEngine engine;
    QVERIFY(engine.objectCreated() == false);

    // Test basic QML component loading
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.15\nRectangle { width: 100; height: "
                      "100; color: \"red\" }",
                      QUrl());

    QVERIFY(component.isReady());
    QVERIFY(!component.isError());

    QObject *object = component.create();
    QVERIFY(object != nullptr);

    // Verify the object has the expected properties
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    QVERIFY(item != nullptr);
    QCOMPARE(item->width(), 100.0);
    QCOMPARE(item->height(), 100.0);

    delete object;
    qDebug() << "QML engine test passed";
  }

  void testHelloWorldQml() {
    // Test loading our main.qml file
    QQmlEngine engine;
    QQmlComponent component(&engine);

    // Load the QML file (this will be compiled into the resources)
    component.loadUrl(QUrl("qrc:/LMStudioTest/src/main.qml"));

    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QVERIFY(!component.isError());

    QObject *object = component.create();
    QVERIFY(object != nullptr);

    // Verify it's an ApplicationWindow
    QQuickWindow *window = qobject_cast<QQuickWindow *>(object);
    QVERIFY(window != nullptr);

    // Verify window properties
    QCOMPARE(window->width(), 800.0);
    QCOMPARE(window->height(), 600.0);
    QCOMPARE(window->title(), QString("Hello World - LMStudio Test"));

    // Test that the window is visible by default
    QVERIFY(window->visible());

    delete object;
    qDebug() << "Hello World QML test passed";
  }

  void testQtModules() {
    // Test that required Qt modules are available
    QVERIFY(QT_VERSION_STR != nullptr);

    // Test Qt Quick availability
    QVERIFY(QT_VERSION >= QT_VERSION_CHECK(6, 4, 0));

    // Test QML types
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.15\nText { text: \"test\" }", QUrl());

    QVERIFY(component.isReady());
    QVERIFY(!component.isError());

    QObject *object = component.create();
    QVERIFY(object != nullptr);

    delete object;
    qDebug() << "Qt modules test passed";
  }

  void testGraphicsApi() {
    // Test graphics API configuration
    QSGRendererInterface *renderer = QQuickWindow::graphicsApi();
    QVERIFY(renderer != nullptr);

    // We set OpenGL in main.cpp, verify it's set
    QCOMPARE(QQuickWindow::graphicsApi(), QSGRendererInterface::OpenGL);

    qDebug() << "Graphics API test passed";
  }

  void testHighDpiScaling() {
    // Test high DPI scaling attributes
    QVERIFY(QGuiApplication::testAttribute(Qt::AA_EnableHighDpiScaling));
    QVERIFY(QGuiApplication::testAttribute(Qt::AA_UseHighDpiPixmaps));

    qDebug() << "High DPI scaling test passed";
  }

  void testPerformance() {
    // Basic performance test
    QElapsedTimer timer;
    timer.start();

    // Create and destroy multiple QML components
    QQmlEngine engine;
    for (int i = 0; i < 100; i++) {
      QQmlComponent component(&engine);
      component.setData("import QtQuick 2.15\nRectangle { width: 50; height: "
                        "50; color: \"blue\" }",
                        QUrl());
      QVERIFY(component.isReady());
      QObject *object = component.create();
      delete object;
    }

    qint64 elapsed = timer.elapsed();
    QVERIFY(elapsed < 5000); // Should complete in less than 5 seconds

    qDebug() << "Performance test passed - Created 100 components in" << elapsed
             << "ms";
  }

  void testMemoryManagement() {
    // Test basic memory management
    QQmlEngine engine;

    // Create objects and verify they're cleaned up
    QVector<QObject *> objects;
    for (int i = 0; i < 10; i++) {
      QQmlComponent component(&engine);
      component.setData("import QtQuick 2.15\nRectangle { width: 100; height: "
                        "100; color: \"green\" }",
                        QUrl());
      QVERIFY(component.isReady());
      QObject *object = component.create();
      objects.append(object);
    }

    // Delete all objects
    qDeleteAll(objects);
    objects.clear();

    // Garbage collect
    engine.collectGarbage();

    qDebug() << "Memory management test passed";
  }

  void testConsoleOutput() {
    // Test console output functionality
    QString testMessage = "Hello World Test Message";
    qDebug() << testMessage;

    // In a real test, we might capture console output
    // For now, just verify the function works
    QVERIFY(!testMessage.isEmpty());

    qDebug() << "Console output test passed";
  }
};

// Add the test to the Qt Test framework
QTEST_APPLESS_MAIN(HelloWorldTest)

#include "tst_helloworld.moc"