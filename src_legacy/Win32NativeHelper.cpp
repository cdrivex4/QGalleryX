#include "Win32NativeHelper.h"
#include "TaskScheduler.h"
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#include <shobjidl.h>
#include <sddl.h>

static WNDPROC g_oldWndProc = nullptr;

static LRESULT CALLBACK NativeSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (msg == WM_DROPFILES) {
    HDROP hDrop = (HDROP)wParam;
    UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);

    QStringList paths;
    for (UINT i = 0; i < count; ++i) {
      wchar_t buf[MAX_PATH * 2];
      if (DragQueryFileW(hDrop, i, buf, MAX_PATH * 2) > 0) {
        QString path = QString::fromWCharArray(buf);
        path = QDir::toNativeSeparators(path);
        paths.append(path);
      }
    }
    DragFinish(hDrop);

    if (!paths.isEmpty()) {
      qDebug() << "[Win32NativeHelper WndProc] WM_DROPFILES Intercepted:" << paths;
      // Dispatch via QMetaObject::invokeMethod to ensure GUI thread emission
      QMetaObject::invokeMethod(&Win32NativeHelper::instance(), [paths]() {
        Win32NativeHelper::instance().handleDropPayload(paths);
      }, Qt::QueuedConnection);
    }
    return 0; // Handled
  } else if (msg == WM_COPYDATA) {
    PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;
    if (pcds && pcds->dwData == 0x51475858 /* 'QGXX' */) {
      const wchar_t *data = (const wchar_t *)pcds->lpData;
      if (data) {
        QString payload = QString::fromWCharArray(data, pcds->cbData / sizeof(wchar_t));
        QStringList paths = payload.split(L'\n', Qt::SkipEmptyParts);
        if (!paths.isEmpty()) {
          qDebug() << "[Win32NativeHelper WndProc] WM_COPYDATA Received:" << paths;
          QMetaObject::invokeMethod(&Win32NativeHelper::instance(), [paths]() {
            Win32NativeHelper::instance().handleDropPayload(paths);
          }, Qt::QueuedConnection);
        }
      }
      return TRUE;
    }
  }

  return CallWindowProcW(g_oldWndProc, hwnd, msg, wParam, lParam);
}
#endif

Win32NativeHelper &Win32NativeHelper::instance() {
  static Win32NativeHelper inst;
  return inst;
}

Win32NativeHelper::Win32NativeHelper(QObject *parent) : QObject(parent) {
  startNamedPipeServer();
}

Win32NativeHelper::~Win32NativeHelper() {
  m_pipeRunning = false;
#ifdef Q_OS_WIN
  if (m_hwnd && m_oldWndProc) {
    SetWindowLongPtrW((HWND)m_hwnd, GWLP_WNDPROC, (LONG_PTR)m_oldWndProc);
  }
#endif
}

void Win32NativeHelper::handleDropPayload(const QStringList &paths) {
  if (paths.isEmpty()) return;
  qDebug() << "[Win32NativeHelper] Dispatching dropped payload to QML:" << paths;
  emit filesDropped(paths);
}

void Win32NativeHelper::registerWindow(QWindow *window) {
  if (!window) return;

#ifdef Q_OS_WIN
  HWND hwnd = (HWND)window->winId();
  m_hwnd = (void*)hwnd;

  if (hwnd) {
    // 1. Enable legacy WM_DROPFILES on the window handle
    DragAcceptFiles(hwnd, TRUE);

    // 2. Bypass UIPI (User Interface Privilege Isolation)
    ChangeWindowMessageFilterEx(hwnd, WM_DROPFILES, MSGFLT_ALLOW, NULL);
    ChangeWindowMessageFilterEx(hwnd, WM_COPYDATA, MSGFLT_ALLOW, NULL);
    ChangeWindowMessageFilterEx(hwnd, 0x0049 /* WM_COPYGLOBALDATA */, MSGFLT_ALLOW, NULL);

    // 3. Subclass Window Procedure to catch drop messages before Qt handles them
    g_oldWndProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)NativeSubclassProc);
    m_oldWndProc = (void*)g_oldWndProc;

    qDebug() << "[Win32NativeHelper] Subclassed HWND:" << hwnd << "successfully for Option 10 Hybrid Drop.";
  }
#endif
}

bool Win32NativeHelper::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) {
#ifdef Q_OS_WIN
  if (eventType == "windows_generic_MSG") {
    MSG *msg = static_cast<MSG *>(message);

    if (msg->message == WM_DROPFILES) {
      HDROP hDrop = (HDROP)msg->wParam;
      UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, NULL, 0);

      QStringList paths;
      for (UINT i = 0; i < count; ++i) {
        wchar_t buf[MAX_PATH * 2];
        if (DragQueryFileW(hDrop, i, buf, MAX_PATH * 2) > 0) {
          QString path = QString::fromWCharArray(buf);
          path = QDir::toNativeSeparators(path);
          paths.append(path);
        }
      }
      DragFinish(hDrop);

      if (!paths.isEmpty()) {
        qDebug() << "[Win32NativeHelper Filter] WM_DROPFILES Event:" << paths;
        handleDropPayload(paths);
      }

      if (result) *result = 0;
      return true; // Event handled
    }
  }
#else
  Q_UNUSED(eventType);
  Q_UNUSED(message);
  Q_UNUSED(result);
#endif
  return false;
}

void Win32NativeHelper::startNamedPipeServer() {
#ifdef Q_OS_WIN
  m_pipeRunning = true;
  TaskScheduler::instance().addTask([this]() {
    const wchar_t *pipeName = L"\\\\.\\pipe\\qgalleryx_drop_pipe";

    while (m_pipeRunning) {
      HANDLE hPipe = CreateNamedPipeW(
          pipeName,
          PIPE_ACCESS_INBOUND,
          PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
          PIPE_UNLIMITED_INSTANCES,
          4096, 4096, 0, NULL);

      if (hPipe == INVALID_HANDLE_VALUE) {
        QThread::msleep(1000);
        continue;
      }

      if (ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED)) {
        wchar_t buffer[4096];
        DWORD bytesRead = 0;
        if (ReadFile(hPipe, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, NULL)) {
          buffer[bytesRead / sizeof(wchar_t)] = L'\0';
          QString payload = QString::fromWCharArray(buffer);
          QStringList paths = payload.split(L'\n', Qt::SkipEmptyParts);
          
          if (!paths.isEmpty()) {
            qDebug() << "[Win32NativeHelper Pipe] Drop Payload Received via Named Pipe:" << paths;
            QMetaObject::invokeMethod(this, [this, paths]() {
              handleDropPayload(paths);
            }, Qt::QueuedConnection);
          }
        }
      }
      DisconnectNamedPipe(hPipe);
      CloseHandle(hPipe);
    }
  }, TaskScheduler::IO_BOUND, TaskScheduler::Low, "NamedPipeDropListener");
#endif
}

void Win32NativeHelper::openFolderPicker(const QString &initialPath) {
#ifdef Q_OS_WIN
  // Run IFileOpenDialog on a background thread so UI thread NEVER stalls over network SMB shares
  TaskScheduler::instance().addTask([this, initialPath]() {
    CoInitialize(NULL);

    IFileOpenDialog *pfd = NULL;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd));

    if (SUCCEEDED(hr)) {
      DWORD dwOptions;
      if (SUCCEEDED(pfd->GetOptions(&dwOptions))) {
        pfd->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
      }

      pfd->SetTitle(L"Select Image Folder");

      if (!initialPath.isEmpty()) {
        QString cleanInit = initialPath;
        if (cleanInit.startsWith("file:///", Qt::CaseInsensitive)) cleanInit = cleanInit.mid(8);
        cleanInit = QDir::toNativeSeparators(cleanInit);

        IShellItem *psiFolder = NULL;
        std::wstring wpath = cleanInit.toStdWString();
        if (SUCCEEDED(SHCreateItemFromParsingName(wpath.c_str(), NULL, IID_PPV_ARGS(&psiFolder)))) {
          pfd->SetFolder(psiFolder);
          psiFolder->Release();
        }
      }

      HWND parentHwnd = (HWND)m_hwnd;
      hr = pfd->Show(parentHwnd);

      if (SUCCEEDED(hr)) {
        IShellItem *psiResult = NULL;
        if (SUCCEEDED(pfd->GetResult(&psiResult))) {
          PWSTR pszPath = NULL;
          if (SUCCEEDED(psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath))) {
            QString selectedPath = QString::fromWCharArray(pszPath);
            selectedPath = QDir::toNativeSeparators(selectedPath);
            CoTaskMemFree(pszPath);

            qDebug() << "[Win32NativeHelper] Native Folder Selected:" << selectedPath;
            emit folderPicked(selectedPath);
          }
          psiResult->Release();
        }
      }
      pfd->Release();
    }
    CoUninitialize();
  }, TaskScheduler::IO_BOUND, TaskScheduler::Immediate, "NativeFolderPicker");
#else
  Q_UNUSED(initialPath);
#endif
}
