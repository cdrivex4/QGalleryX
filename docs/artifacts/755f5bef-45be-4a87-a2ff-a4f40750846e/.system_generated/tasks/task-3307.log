#include "FastImageItem.h"
#include "../../src/AsyncImageProvider.h"
#include <QSGSimpleTextureNode>

FastImageItem::FastImageItem(QQuickItem *parent) : QQuickItem(parent) {
  setFlag(ItemHasContents, true);
}

FastImageItem::~FastImageItem() {
  if (m_response) {
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

  // Synchronous cache hit check bypasses QML entirely
  QImage cached = AsyncImageProvider::getCachedImage(id, m_sourceSize);
  if (!cached.isNull()) {
    AsyncImageProvider::s_cacheHits++;
    m_image = cached;
    m_dirtyTexture = true;
    m_isLoading = false;
    emit isLoadingChanged();
    update();
    return;
  }

  // Cache miss, schedule async load
  m_isLoading = true;
  emit isLoadingChanged();
  AsyncImageProvider::s_cacheMisses++;
  static AsyncImageProvider provider;
  m_response = static_cast<AsyncImageResponse *>(provider.requestImageResponse(id, m_sourceSize));
  
  // Direct connection ensures immediate handling without event loop delays
  connect(m_response, &QQuickImageResponse::finished, this, [this]() {
    if (m_response) {
        // We modified AsyncImageResponse to expose m_image
        m_image = m_response->m_image;
        m_dirtyTexture = true;
        m_response->deleteLater();
        m_response = nullptr;
        m_isLoading = false;
        emit isLoadingChanged();
        update();
    }
  }, Qt::DirectConnection);
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
  // We could reload here, but usually it's set before source
}

QSGNode *FastImageItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) {
  QSGSimpleTextureNode *node = static_cast<QSGSimpleTextureNode *>(oldNode);

  if (m_image.isNull()) {
    delete node;
    return nullptr;
  }

  if (!node) {
    node = new QSGSimpleTextureNode();
  }

  if (m_dirtyTexture) {
    if (m_texture) {
      delete m_texture;
    }
    m_texture = window()->createTextureFromImage(m_image, QQuickWindow::TextureIsOpaque);
    m_dirtyTexture = false;
  }

  if (m_texture) {
    node->setTexture(m_texture);
    
    // Calculate aspect ratio preserving crop
    QRectF targetRect(0, 0, width(), height());
    
    if (m_fillMode == 1) { // PreserveAspectCrop
      double itemAspect = width() / height();
      double imageAspect = (double)m_image.width() / m_image.height();
      
      QRectF sourceRect(0, 0, 1, 1);
      if (imageAspect > itemAspect) {
        // Image is wider, crop sides
        double cropWidth = itemAspect / imageAspect;
        sourceRect.setX((1.0 - cropWidth) / 2.0);
        sourceRect.setWidth(cropWidth);
      } else {
        // Image is taller, crop top/bottom
        double cropHeight = imageAspect / itemAspect;
        sourceRect.setY((1.0 - cropHeight) / 2.0);
        sourceRect.setHeight(cropHeight);
      }
      node->setSourceRect(sourceRect);
    }
    
    node->setRect(targetRect);
  }

  return node;
}
