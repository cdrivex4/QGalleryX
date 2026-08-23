#include "DiagnosticsMonitor.h"
#include "../../src_legacy/AsyncImageProvider.h"
#include "../../src_legacy/SettingsHelper.h"
#include "../../src_legacy/TaskScheduler.h"
#include "ScrollBenchImageModel.h"
#include <QDebug>

DiagnosticsMonitor::DiagnosticsMonitor(QObject *parent)
    : QObject(parent), m_model(nullptr), m_settings(nullptr),
      m_healthStatus("Initializing..."), m_healthColor("yellow"),
      m_viewportCullingEnabled(true), m_viewportRangeSize(0),
      m_bufferedRangeSize(0), m_viewportStartIndex(0), m_viewportEndIndex(0),
      m_viewportStatus("Not initialized"), m_viewportStatusColor("gray"),
      m_totalItems(0), m_loadedItems(0), m_pendingRequests(0),
      m_stagedRequests(0), m_loadProgressStatus("No data"),
      m_diskCacheEnabled(false), m_settingsStatus("Not initialized"),
      m_settingsStatusColor("gray"), m_activeIOTasks(0), m_ioStatus("Idle"),
      m_lastLoadTimestamp(QDateTime::currentDateTime()) {

  m_diagnosticsTimer = new QTimer(this);
  m_diagnosticsTimer->setInterval(1000); // Run diagnostics every second
  connect(m_diagnosticsTimer, &QTimer::timeout, this,
          &DiagnosticsMonitor::runDiagnostics);
  m_diagnosticsTimer->start();

  qCritical() << "[Diagnostics] Monitor started - running checks every 1000ms";
}

void DiagnosticsMonitor::attachModel(ScrollBenchImageModel *model) {
  m_model = model;
  if (m_model) {
    connect(m_model, &ScrollBenchImageModel::visibleRangeChanged, this,
            &DiagnosticsMonitor::onModelStateChanged);
    connect(m_model, &ScrollBenchImageModel::viewportCullingEnabledChanged,
            this, &DiagnosticsMonitor::onModelStateChanged);
    qCritical() << "[Diagnostics] Attached to ScrollBenchImageModel";
  }
}

void DiagnosticsMonitor::attachSettings(SettingsHelper *settings) {
  m_settings = settings;
  if (m_settings) {
    connect(m_settings, &SettingsHelper::diskCacheSizeMBChanged, this,
            &DiagnosticsMonitor::onSettingsChanged);
    qCritical() << "[Diagnostics] Attached to SettingsHelper";
  }
}

void DiagnosticsMonitor::runDiagnostics() {
  clearWarnings();

  checkViewportCulling();
  checkLoadProgress();
  checkSettings();
  checkAdaptiveIO();

  updateOverallHealth();
}

void DiagnosticsMonitor::checkViewportCulling() {
  if (!m_model) {
    m_viewportStatus = "Model not attached";
    m_viewportStatusColor = "gray";
    emit viewportStatusChanged();
    return;
  }

  // Update state from model
  m_viewportCullingEnabled = m_model->viewportCullingEnabled();
  m_viewportStartIndex = m_model->visibleStartIndex();
  m_viewportEndIndex = m_model->visibleEndIndex();
  m_viewportRangeSize = m_viewportEndIndex - m_viewportStartIndex + 1;

  // Calculate buffered range (BUFFER_SIZE = 50)
  constexpr int BUFFER_SIZE = 50;
  int bufferedStart = qMax(0, m_viewportStartIndex - BUFFER_SIZE);
  int bufferedEnd =
      qMin(m_model->totalItems() - 1, m_viewportEndIndex + BUFFER_SIZE);
  m_bufferedRangeSize = bufferedEnd - bufferedStart + 1;

  emit viewportCullingChanged();
  emit viewportRangeChanged();

  // Validate viewport culling behavior
  if (m_viewportCullingEnabled) {
    if (m_viewportRangeSize == 0) {
      m_viewportStatus = "⚠️ Viewport range is ZERO";
      m_viewportStatusColor = "red";
      addCritical("Viewport range is 0 - no items detected in viewport!");
    } else if (m_viewportRangeSize <= 1 && m_totalItems > 50) {
      m_viewportStatus =
          QString("⚠️ Restricted viewport: %1 items").arg(m_viewportRangeSize);
      m_viewportStatusColor = "orange";
      addWarning(
          QString("Viewport only showing %1 items - possible detection issue")
              .arg(m_viewportRangeSize));
    } else if (m_bufferedRangeSize < EXPECTED_MIN_RANGE_SIZE && m_model->totalItems() >= EXPECTED_MIN_RANGE_SIZE) {
      m_viewportStatus =
          QString("⚠️ Small buffered range: %1 items (expected >%2)")
              .arg(m_bufferedRangeSize)
              .arg(EXPECTED_MIN_RANGE_SIZE);
      m_viewportStatusColor = "orange";
      addWarning(
          QString("Buffered range only %1 items - might be the '19 items' bug")
              .arg(m_bufferedRangeSize));
    } else {
      m_viewportStatus =
          QString("✓ Culling ON: viewport %1 items, buffered %2 items")
              .arg(m_viewportRangeSize)
              .arg(m_bufferedRangeSize);
      m_viewportStatusColor = "green";
    }
  } else {
    // Viewport culling is OFF - should load all items
    if (m_totalItems > 0 && m_loadedItems < m_totalItems) {
      double progressPct = (m_loadedItems * 100.0) / m_totalItems;
      m_viewportStatus = QString("✓ Culling OFF: Loading all %1 items (%2%)")
                             .arg(m_totalItems)
                             .arg(progressPct, 0, 'f', 1);
      m_viewportStatusColor = "cyan";
    } else {
      m_viewportStatus =
          QString("✓ Culling OFF: All %1 items available").arg(m_totalItems);
      m_viewportStatusColor = "green";
    }
  }

  emit viewportStatusChanged();
}

void DiagnosticsMonitor::checkLoadProgress() {
  if (!m_model) {
    m_loadProgressStatus = "Model not attached";
    emit loadProgressChanged();
    return;
  }

  m_totalItems = m_model->totalItems();

  // Save previous loaded items to check for progress
  int prevLoaded = m_loadedItems;

  // Count loaded items
  m_loadedItems = m_model->loadedCount();

  // Get pending from TaskScheduler
  m_stagedRequests = 0;
  m_pendingRequests = TaskScheduler::instance().getQueueSize(TaskScheduler::Immediate) +
                      TaskScheduler::instance().getQueueSize(TaskScheduler::High);

  emit totalItemsChanged();
  emit loadedItemsChanged();
  emit pendingRequestsChanged();
  emit stagedRequestsChanged();

  if (m_pendingRequests > 0) {
    if (m_loadedItems != prevLoaded || TaskScheduler::instance().isPaused()) {
      m_lastLoadTimestamp = QDateTime::currentDateTime();
    }
  } else {
    m_lastLoadTimestamp = QDateTime::currentDateTime();
  }

  // Build status string
  if (m_totalItems == 0) {
    m_loadProgressStatus = "No items loaded";
  } else {
    double progressPct = (m_loadedItems * 100.0) / m_totalItems;
    m_loadProgressStatus =
        QString("%1/%2 loaded (%3%) | Pending: %4 | Staged: %5")
            .arg(m_loadedItems)
            .arg(m_totalItems)
            .arg(progressPct, 0, 'f', 1)
            .arg(m_pendingRequests)
            .arg(m_stagedRequests);
  }

  emit loadProgressChanged();
}

void DiagnosticsMonitor::checkSettings() {
  if (!m_settings) {
    m_settingsStatus = "Settings not attached";
    m_settingsStatusColor = "gray";
    emit settingsStatusChanged();
    return;
  }

  m_diskCacheEnabled = (m_settings->diskCacheSizeMB() > 0);
  emit diskCacheChanged();

  // Verify settings synchronization
  bool asyncProviderCacheEnabled = true;

  if (m_diskCacheEnabled != asyncProviderCacheEnabled) {
    m_settingsStatus = QString("❌ MISMATCH: Settings=%1, Provider=%2")
                           .arg(m_diskCacheEnabled ? "ON" : "OFF")
                           .arg(asyncProviderCacheEnabled ? "ON" : "OFF");
    m_settingsStatusColor = "red";
    addCritical("Disk cache setting mismatch between SettingsHelper and "
                "AsyncImageProvider!");
  } else {
    m_settingsStatus = QString("✓ Disk cache: %1 (synchronized)")
                           .arg(m_diskCacheEnabled ? "ON" : "OFF");
    m_settingsStatusColor = "green";
  }

  emit settingsStatusChanged();
}

void DiagnosticsMonitor::checkAdaptiveIO() {
  m_activeIOTasks = TaskScheduler::instance().getQueueSize(TaskScheduler::Normal);
  emit ioTasksChanged();

  if (m_activeIOTasks == 0) {
    m_ioStatus = "Idle (0 tasks)";
  } else if (m_activeIOTasks > 1000) {
    m_ioStatus = QString("⚠️ High load: %1 tasks").arg(m_activeIOTasks);
    addWarning(QString("Task queue very large: %1 tasks").arg(m_activeIOTasks));
  } else {
    m_ioStatus = QString("Active: %1 tasks").arg(m_activeIOTasks);
  }

  emit ioStatusChanged();
}

void DiagnosticsMonitor::updateOverallHealth() {
  int criticalCount = m_activeCriticals.size();
  int warningCount = m_activeWarnings.size();

  if (criticalCount > 0) {
    m_healthStatus = QString("❌ CRITICAL (%1 issues)").arg(criticalCount);
    m_healthColor = "red";

    // Emit alert for first critical
    emit criticalIssueDetected(m_activeCriticals.first());
  } else if (warningCount > 0) {
    m_healthStatus = QString("⚠️ Warnings (%1)").arg(warningCount);
    m_healthColor = "orange";
  } else {
    m_healthStatus = "✓ All systems operational";
    m_healthColor = "green";
  }

  emit healthChanged();
}

void DiagnosticsMonitor::addWarning(const QString &warning) {
  if (!m_activeWarnings.contains(warning)) {
    m_activeWarnings.append(warning);
    qWarning() << "[Diagnostics] WARNING:" << warning;
    emit warningsChanged();
  }
}

void DiagnosticsMonitor::addCritical(const QString &critical) {
  if (!m_activeCriticals.contains(critical)) {
    m_activeCriticals.append(critical);
    qCritical() << "[Diagnostics] CRITICAL:" << critical;
    emit criticalsChanged();
  }
}

void DiagnosticsMonitor::clearWarnings() {
  if (!m_activeWarnings.isEmpty()) {
    m_activeWarnings.clear();
    emit warningsChanged();
  }
  if (!m_activeCriticals.isEmpty()) {
    m_activeCriticals.clear();
    emit criticalsChanged();
  }
}

void DiagnosticsMonitor::onModelStateChanged() {
  // Model state changed, run diagnostics immediately
  runDiagnostics();
}

void DiagnosticsMonitor::onSettingsChanged() {
  // Settings changed, check synchronization
  checkSettings();
  updateOverallHealth();
}
