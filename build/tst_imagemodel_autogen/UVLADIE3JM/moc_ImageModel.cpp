/****************************************************************************
** Meta object code from reading C++ file 'ImageModel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/ImageModel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ImageModel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.9.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10ImageModelE_t {};
} // unnamed namespace

template <> constexpr inline auto ImageModel::qt_create_metaobjectdata<qt_meta_tag_ZN10ImageModelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "ImageModel",
        "scanDirectory",
        "",
        "path",
        "cropImage",
        "index",
        "cropRect",
        "ImageRoles",
        "FilePathRole",
        "FileNameRole",
        "DateSectionRole",
        "SectionDayRole",
        "SectionMonthRole",
        "SectionYearRole",
        "SectionWeekRole",
        "ExifRole"
    };

    QtMocHelpers::UintData qt_methods {
        // Method 'scanDirectory'
        QtMocHelpers::MethodData<void(const QString &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'cropImage'
        QtMocHelpers::MethodData<bool(int, const QRectF &)>(4, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::Int, 5 }, { QMetaType::QRectF, 6 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'ImageRoles'
        QtMocHelpers::EnumData<enum ImageRoles>(7, 7, QMC::EnumFlags{}).add({
            {    8, ImageRoles::FilePathRole },
            {    9, ImageRoles::FileNameRole },
            {   10, ImageRoles::DateSectionRole },
            {   11, ImageRoles::SectionDayRole },
            {   12, ImageRoles::SectionMonthRole },
            {   13, ImageRoles::SectionYearRole },
            {   14, ImageRoles::SectionWeekRole },
            {   15, ImageRoles::ExifRole },
        }),
    };
    return QtMocHelpers::metaObjectData<ImageModel, qt_meta_tag_ZN10ImageModelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject ImageModel::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractListModel::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10ImageModelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10ImageModelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10ImageModelE_t>.metaTypes,
    nullptr
} };

void ImageModel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<ImageModel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->scanDirectory((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 1: { bool _r = _t->cropImage((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QRectF>>(_a[2])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
}

const QMetaObject *ImageModel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *ImageModel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10ImageModelE_t>.strings))
        return static_cast<void*>(this);
    return QAbstractListModel::qt_metacast(_clname);
}

int ImageModel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    return _id;
}
QT_WARNING_POP
