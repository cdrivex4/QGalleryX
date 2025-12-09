#include "AsyncImageProvider.h"
#include "FrameBudgetScheduler.h"
#include "ScrollBenchImageModel.h"
#include "TelemetryMonitor.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>


int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  QQmlApplicationEngine engine;

  // Register async image provider for real images
  engine.addImageProvider(QLatin1String("async"), new AsyncImageProvider());

  // Create core components
  ScrollBenchImageModel imageModel;
  FrameBudgetScheduler frameBudget;
  TelemetryMonitor telemetry;

  // Generate 10,000 test items by default
  imageModel.generateTestData(10000);

  // Connect model to telemetry
  QObject::connect(
      &imageModel, &ScrollBenchImageModel::pendingDecodeCountChanged,
      &telemetry, [&telemetry, &imageModel]() {
        telemetry.setPendingDecodes(imageModel.pendingDecodeCount());
      });

  // Expose to QML
  engine.rootContext()->setContextProperty("imageModel", &imageModel);
  engine.rootContext()->setContextProperty("frameBudget", &frameBudget);
  engine.rootContext()->setContextProperty("telemetry", &telemetry);

  // Load QML
  const QUrl url(u"qrc:/ScrollBench/qml/MainScrollBench.qml"_qs);
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
