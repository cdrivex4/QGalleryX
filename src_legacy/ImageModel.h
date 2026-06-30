#ifndef IMAGEMODEL_H
#define IMAGEMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QList>
#include <QObject>
#include <QRectF>
#include <QString>
#include <QVector>

struct ImageInfo {
  QString filePath;
  QString fileName;
  QDateTime date;
  QDateTime dateTaken; // Added for sorting
  QDateTime dateModified;
  qint64 size = 0; // Added size member
};

class ImageModel : public QAbstractListModel {
  Q_OBJECT

  Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
  enum ImageRoles {
    FilePathRole = Qt::UserRole + 1,
    FileNameRole,
    DateSectionRole,
    SectionDayRole,
    SectionMonthRole,
    SectionYearRole,
    SectionWeekRole,
    ExifRole,
    IsRawRole
  };
  Q_ENUM(ImageRoles)

  explicit ImageModel(QObject *parent = nullptr);

  Q_INVOKABLE void scanDirectory(const QString &path);
  Q_INVOKABLE bool cropImage(int index, const QRectF &cropRect);
  Q_INVOKABLE QVariantMap getMetadata(int index);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  bool isLoading() const { return m_isLoading; }

signals:
  void isLoadingChanged();

private:
  QList<ImageInfo> m_images;
  bool m_isLoading = false;
};

#endif // IMAGEMODEL_H
