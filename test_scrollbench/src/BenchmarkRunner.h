#ifndef BENCHMARKRUNNER_H
#define BENCHMARKRUNNER_H

#include <QObject>
#include <QString>
#include <QVariantMap>

class BenchmarkRunner : public QObject {
  Q_OBJECT
  Q_PROPERTY(bool isRunning READ isRunning NOTIFY isRunningChanged)
  Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
  Q_PROPERTY(QVariantMap results READ results NOTIFY resultsChanged)

public:
  explicit BenchmarkRunner(QObject *parent = nullptr);

  bool isRunning() const { return m_isRunning; }
  QString statusText() const { return m_statusText; }
  QVariantMap results() const { return m_results; }

  Q_INVOKABLE void runDriveBenchmark(const QString &rootPath = "C:/", int targetWorkingSet = 4840);

signals:
  void isRunningChanged();
  void statusTextChanged();
  void resultsChanged();
  void benchmarkCompleted(const QVariantMap &summary);

private:
  bool m_isRunning = false;
  QString m_statusText;
  QVariantMap m_results;
};

#endif // BENCHMARKRUNNER_H
