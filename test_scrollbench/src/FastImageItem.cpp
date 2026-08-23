#include "FastImageItem.h"
#include "BC1Engine.h"
#include "FileCacheManager.h"
#include "AsyncImageProvider.h"
#include <QSGSimpleTextureNode>

FastImageItem::FastImageItem(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);
}

FastImageItem::~FastImageItem() {
  if (m_response) {
    disconnect(m_response, nullptr, this, nullptr);
    m_response->cancel();
    m_response->deleteLater();
    m_response = nullptr;
  }
}

void FastImageItem::setSource(const QString &source) {
  if (m_source == source)
    return;
  m_source = source;
  emit sourceChanged();

  // Cancel any pending request
  if (m_response) {
    disconnect(m_response, nullptr, this, nullptr);
    m_response->cancel();
    m_response->deleteLater();
    m_response = nullptr;
  }

  if (m_source.isEmpty()) {
    m_image = QImage();
    m_dirtyTexture = true;
    m_isLoading = false;
    emit isLoadingChanged();
    update();
    return;
  }

  // Use AsyncImageProvider to get the image
  QString id = m_source;
  if (id.startsWith("image://async/")) {
    id = id.mid(14);
  }

  // 1. Synchronous L1 RAM Cache hit check (0ms instant)
  QImage cached = AsyncImageProvider::getCachedImage(id, m_sourceSize);
  if (!cached.isNull()) {
    m_image = cached;
    m_dirtyTexture = true;
    m_isLoading = false;
    emit isLoadingChanged();
    update();
    return;
  }

  // 2. Synchronous L2 .mmap BC1 Hardware Texture check (<0.05ms SIMD decode)
  QByteArray mmapData = FileCacheManager::instance().getCachedData(id, m_sourceSize);
  if (!mmapData.isEmpty()) {
    QImage l2Image;
    if (mmapData.startsWith("BC1_") && mmapData.size() >= 8) {
      quint16 w = *reinterpret_cast<const quint16 *>(mmapData.constData() + 4);
      quint16 h = *reinterpret_cast<const quint16 *>(mmapData.constData() + 6);
      l2Image = BC1Engine::decompressImage(mmapData.mid(8), w, h);
    } else {
      l2Image.loadFromData(mmapData);
    }
    if (!l2Image.isNull()) {
      AsyncImageProvider::insertCachedImage(id, l2Image, m_sourceSize);
      m_image = l2Image;
      m_dirtyTexture = true;
      m_isLoading = false;
      emit isLoadingChanged();
      update();
      return;
    }
  }

  // Cache miss, schedule async load
  m_isLoading = true;
  emit isLoadingChanged();
  static auto *provider = new AsyncImageProvider();
  m_response = static_cast<AsyncImageResponse *>(provider->requestImageResponse(id, m_sourceSize));
  
  // Use QueuedConnection: the finished() signal is emitted from the main
  // thread (via FrameBudgetScheduler or QueuedConnection invokeMethod).
  // Using QueuedConnection here ensures that if this FastImageItem is
  // destroyed between the emit and the slot execution, Qt will automatically
  // discard the pending slot invocation (because the receiver is gone).
  // DirectConnection would execute inline during emit, which races with
  // delegate destruction during rapid scroll/zoom.
  connect(m_response, &QQuickImageResponse::finished, this, [this, response = m_response]() {
    if (m_response != response)
        return; // Stale response from a previous setSource call
    m_image = m_response->image();
    m_dirtyTexture = true;
    m_response->deleteLater();
    m_response = nullptr;
    m_isLoading = false;
    emit isLoadingChanged();
    update();
  }, Qt::QueuedConnection);
}

void FastImageItem::setFillMode(int mode) {
  if (m_fillMode == mode) return;
  m_fillMode = mode;
  emit fillModeChanged();
  update();
}

void FastImageItem::setSourceSize(const QSize &size) {
  if (m_sourceSize == size) return;
  m_sourceSize = size;
  emit sourceSizeChanged();

  if (!m_source.isEmpty()) {
    // Clear the currently displayed image immediately so the delegate shows
    // a blank/loading state rather than stretching the old-size image.
    m_image = QImage();
    m_dirtyTexture = true;
    update();

    QString tmp = m_source;
    m_source.clear(); // Force a fresh request at the new size
    setSource(tmp);
  }
}

QSGNode *FastImageItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
  QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>(oldNode);

  if (m_image.isNull()) {
    // Node owns the texture via setOwnsTexture(true), so deleting the node
    // will free the GPU texture. We must NOT delete m_texture separately.
    delete oldNode;
    m_texture = nullptr;
    return nullptr;
  }

  if (!node) {
    node = new QSGSimpleTextureNode();
    node->setOwnsTexture(true);
  }

  if (m_dirtyTexture) {
    // When setOwnsTexture(true), the node will delete the OLD texture when
    // we call setTexture() with a new one. So we must create the new texture
    // FIRST, then hand it to setTexture() which will delete the old one.
    QSGTexture *newTexture = window()->createTextureFromImage(
        m_image,
        QQuickWindow::CreateTextureOptions(
            QQuickWindow::TextureIsOpaque | QQuickWindow::TextureCanUseAtlas));
    node->setTexture(newTexture);
    m_texture = newTexture;
    m_dirtyTexture = false;
    
    // Calculate aspect ratio preserving crop
    QRectF targetRect(0, 0, width(), height());
    
    if (m_fillMode == 1) { // PreserveAspectCrop
      double itemAspect = width() / height();
      double imageAspect = (double)m_image.width() / m_image.height();
      
      QRectF sourceRect(0, 0, m_image.width(), m_image.height());
      if (imageAspect > itemAspect) {
        // Image is wider, crop sides
        double cropWidth = m_image.height() * itemAspect;
        sourceRect.setX((m_image.width() - cropWidth) / 2.0);
        sourceRect.setWidth(cropWidth);
      } else {
        // Image is taller, crop top/bottom
        double cropHeight = m_image.width() / itemAspect;
        sourceRect.setY((m_image.height() - cropHeight) / 2.0);
        sourceRect.setHeight(cropHeight);
      }
      node->setSourceRect(sourceRect);
    }
    
    node->setRect(targetRect);
  } else if (m_texture) {
    // Only update geometry, texture hasn't changed
    QRectF targetRect(0, 0, width(), height());
    
    if (m_fillMode == 1) {
      double itemAspect = width() / height();
      double imageAspect = (double)m_image.width() / m_image.height();
      
      QRectF sourceRect(0, 0, m_image.width(), m_image.height());
      if (imageAspect > itemAspect) {
        double cropWidth = m_image.height() * itemAspect;
        sourceRect.setX((m_image.width() - cropWidth) / 2.0);
        sourceRect.setWidth(cropWidth);
      } else {
        double cropHeight = m_image.width() / itemAspect;
        sourceRect.setY((m_image.height() - cropHeight) / 2.0);
        sourceRect.setHeight(cropHeight);
      }
      node->setSourceRect(sourceRect);
    }
    
    node->setRect(targetRect);
  }

  return node;
}
