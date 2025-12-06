#include "../src/ImageModel.h"
#include <QSignalSpy>
#include <QtTest>

class TestImageModel : public QObject {
  Q_OBJECT

private slots:
  void testScanning() {
    fprintf(stderr, "[DEBUG] Starting testScanning...\n");
    ImageModel model;
    fprintf(stderr, "[DEBUG] ImageModel created.\n");

    // Create a temporary directory with some dummy files
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile file1(tempDir.path() + "/test1.jpg");
    file1.open(QIODevice::WriteOnly);
    file1.close();

    QFile file2(tempDir.path() + "/test2.mp4");
    file2.open(QIODevice::WriteOnly);
    file2.close();

    // Use QSignalSpy to wait for scanning to complete (isLoading -> false)
    QSignalSpy loadingSpy(&model, &ImageModel::isLoadingChanged);

    model.scanDirectory(tempDir.path());

    // Wait until isLoading becomes false.
    // It might emit true first, so we might need to wait multiple times.
    // simpler: check model.isLoading(), if true, wait.

    // We expect at least one signal (true->false or just false if already
    // running?) scanDirectory sets m_isLoading=true synchronously inside if not
    // already. wait, scanDirectory calls emit isLoadingChanged() synchronously
    // at start. Then async thread runs. Then async thread invokes method on
    // main thread to set m_isLoading=false.

    // So we should see:
    // 1. isLoadingChanged (true) - maybe emitted before we spy if we call scan
    // first? No, we scan AFTER spy.

    // Actually, scanDirectory emits isLoadingChanged(true) sync.
    // Then thread finishes, emits isLoadingChanged(false) async.

    // So we wait for the spy to get 2 signals? or just wait until check is
    // false?

    // Let's loop wait
    while (model.isLoading()) {
      QTest::qWait(100);
    }

    // Just to be safe, process events
    QCoreApplication::processEvents();

    // Verify results
    QCOMPARE(model.rowCount(), 2);

    // Check data
    QModelIndex idx0 = model.index(0, 0);
    QModelIndex idx1 = model.index(1, 0);

    QString path0 = model.data(idx0, ImageModel::FilePathRole).toString();
    QString path1 = model.data(idx1, ImageModel::FilePathRole).toString();

    QVERIFY(path0.contains("test"));
    QVERIFY(path1.contains("test"));
  }
};

QTEST_MAIN(TestImageModel)
#include "tst_imagemodel.moc"
