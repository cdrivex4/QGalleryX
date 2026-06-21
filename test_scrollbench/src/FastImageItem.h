#ifndef FASTIMAGEITEM_H
#define FASTIMAGEITEM_H

#include <QQuickItem>
#include <QImage>
#include <QString>
#include <QQuickWindow>
#include <QSGTexture>

class FastImageItem : public QQuickItem {
  Q_OBJECT
  Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
  Q_PROPERTY(int fillMode READ fillMode WRITE setFillMode NOTIFY fillModeChanged)
  Q_PROPERTY(QSize sourceSize READ sourceSize WRITE setSourceSize NOTIFY sourceSizeChanged)
  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
  explicit FastImageItem(QQuickItem *parent = nullptr);
  ~FastImageItem() override;

  QString source() const { return m_source; }
  void setSource(const QString &source);

  int fillMode() const { return m_fillMode; }
  void setFillMode(int mode);

  QSize sourceSize() const { return m_sourceSize; }
  void setSourceSize(const QSize &size);

  bool isLoading() const { return m_isLoading; }

protected:
  QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData) override;

signals:
  void sourceChanged();
  void fillModeChanged();
  void sourceSizeChanged();
  void isLoadingChanged();

private:
  QString m_source;
  int m_fillMode = 1; // PreserveAspectCrop
  QSize m_sourceSize;
  bool m_isLoading = false;
  
  QImage m_image;
  bool m_dirtyTexture = false;
  QSGTexture *m_texture = nullptr;
  
  class AsyncImageResponse *m_response = nullptr;
};

#endif // FASTIMAGEITEM_H
