#ifndef WIN32NATIVEHELPER_H
#define WIN32NATIVEHELPER_H

#include <QObject>
#include <QAbstractNativeEventFilter>
#include <QStringList>
#include <QWindow>

/**
 * @brief Option 10: Hybrid Master Architecture for 100% Bulletproof Drag & Drop.
 * Combines Win32 WndProc Subclassing, Named Pipe Listener, and Medium-Integrity IPC Proxy.
 */
class Win32NativeHelper : public QObject, public QAbstractNativeEventFilter {
  Q_OBJECT
public:
  static Win32NativeHelper &instance();

  void registerWindow(QWindow *window);
  Q_INVOKABLE void openFolderPicker(const QString &initialPath = "");
  
  // Directly trigger drop handling from C++ or IPC
  void handleDropPayload(const QStringList &paths);

  // QAbstractNativeEventFilter interface
  bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
  void filesDropped(const QStringList &paths);
  void folderPicked(const QString &path);

private:
  explicit Win32NativeHelper(QObject *parent = nullptr);
  ~Win32NativeHelper() override;

  void startNamedPipeServer();
  void checkAndLaunchProxy();

  void *m_hwnd = nullptr; // HWND
  void *m_oldWndProc = nullptr; // WNDPROC
  bool m_pipeRunning = false;
};

#endif // WIN32NATIVEHELPER_H
