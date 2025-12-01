#include "../src/ImageModel.h"
#include <QSignalSpy>
#include <QtTest>


class TestImageModel : public QObject {
  Q_OBJECT

private slots:
  void testScanning() {
    ImageModel model;

    // Create a temporary directory with some dummy files
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());

    QFile file1(tempDir.path() + "/test1.jpg");
    file1.open(QIODevice::WriteOnly);
    file1.close();

    QFile file2(tempDir.path() + "/test2.mp4");
    file2.open(QIODevice::WriteOnly);
    file2.close();

    // Use QSignalSpy to wait for model reset (async scanning)
    QSignalSpy spy(&model, &QAbstractListModel::modelReset);

    model.scanDirectory(tempDir.path());

    // Wait for the async operation to finish (up to 5 seconds)
    QVERIFY(spy.wait(5000));

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
