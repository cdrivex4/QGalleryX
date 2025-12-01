/****************************************************************************
** Meta object code from reading C++ file 'SettingsHelper.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.9.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/SettingsHelper.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SettingsHelper.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN14SettingsHelperE_t {};
} // unnamed namespace

template <> constexpr inline auto SettingsHelper::qt_create_metaobjectdata<qt_meta_tag_ZN14SettingsHelperE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SettingsHelper",
        "graphicsApiChanged",
        "",
        "graphicsDriverChanged",
        "graphicsProfileChanged",
        "selectedApiChanged",
        "thumbnailSizeChanged",
        "gridSizeChanged",
        "cacheSizeMBChanged",
        "concurrentThreadsChanged",
        "restartApp",
        "isApiSupported",
        "apiValue",
        "getCacheStats",
        "QVariantMap",
        "getGpuName",
        "window",
        "refreshGraphicsInfo",
        "graphicsApi",
        "graphicsDriver",
        "graphicsProfile",
        "selectedApi",
        "thumbnailSize",
        "gridSize",
        "cacheSizeMB",
        "concurrentThreads"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'graphicsApiChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'graphicsDriverChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'graphicsProfileChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'selectedApiChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'thumbnailSizeChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'gridSizeChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'cacheSizeMBChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'concurrentThreadsChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'restartApp'
        QtMocHelpers::MethodData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'isApiSupported'
        QtMocHelpers::MethodData<bool(int)>(11, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::Int, 12 },
        }}),
        // Method 'getCacheStats'
        QtMocHelpers::MethodData<QVariantMap()>(13, 2, QMC::AccessPublic, 0x80000000 | 14),
        // Method 'getGpuName'
        QtMocHelpers::MethodData<QString(QObject *)>(15, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QObjectStar, 16 },
        }}),
        // Method 'getGpuName'
        QtMocHelpers::MethodData<QString()>(15, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::QString),
        // Method 'refreshGraphicsInfo'
        QtMocHelpers::MethodData<void(QObject *)>(17, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QObjectStar, 16 },
        }}),
        // Method 'refreshGraphicsInfo'
        QtMocHelpers::MethodData<void()>(17, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'graphicsApi'
        QtMocHelpers::PropertyData<QString>(18, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'graphicsDriver'
        QtMocHelpers::PropertyData<QString>(19, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'graphicsProfile'
        QtMocHelpers::PropertyData<QString>(20, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
        // property 'selectedApi'
        QtMocHelpers::PropertyData<int>(21, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 3),
        // property 'thumbnailSize'
        QtMocHelpers::PropertyData<int>(22, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 4),
        // property 'gridSize'
        QtMocHelpers::PropertyData<int>(23, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
        // property 'cacheSizeMB'
        QtMocHelpers::PropertyData<int>(24, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 6),
        // property 'concurrentThreads'
        QtMocHelpers::PropertyData<int>(25, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 7),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SettingsHelper, qt_meta_tag_ZN14SettingsHelperE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SettingsHelper::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14SettingsHelperE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14SettingsHelperE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN14SettingsHelperE_t>.metaTypes,
    nullptr
} };

void SettingsHelper::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SettingsHelper *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->graphicsApiChanged(); break;
        case 1: _t->graphicsDriverChanged(); break;
        case 2: _t->graphicsProfileChanged(); break;
        case 3: _t->selectedApiChanged(); break;
        case 4: _t->thumbnailSizeChanged(); break;
        case 5: _t->gridSizeChanged(); break;
        case 6: _t->cacheSizeMBChanged(); break;
        case 7: _t->concurrentThreadsChanged(); break;
        case 8: _t->restartApp(); break;
        case 9: { bool _r = _t->isApiSupported((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = std::move(_r); }  break;
        case 10: { QVariantMap _r = _t->getCacheStats();
            if (_a[0]) *reinterpret_cast< QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 11: { QString _r = _t->getGpuName((*reinterpret_cast< std::add_pointer_t<QObject*>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 12: { QString _r = _t->getGpuName();
            if (_a[0]) *reinterpret_cast< QString*>(_a[0]) = std::move(_r); }  break;
        case 13: _t->refreshGraphicsInfo((*reinterpret_cast< std::add_pointer_t<QObject*>>(_a[1]))); break;
        case 14: _t->refreshGraphicsInfo(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SettingsHelper::*)()>(_a, &SettingsHelper::graphicsApiChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsHelper::*)()>(_a, &SettingsHelper::graphicsDriverChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsHelper::*)()>(_a, &SettingsHelper::graphicsProfileChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsHelper::*)()>(_a, &SettingsHelper::selectedApiChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsHelper::*)()>(_a, &SettingsHelper::thumbnailSizeChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsHelper::*)()>(_a, &SettingsHelper::gridSizeChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsHelper::*)()>(_a, &SettingsHelper::cacheSizeMBChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (SettingsHelper::*)()>(_a, &SettingsHelper::concurrentThreadsChanged, 7))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->graphicsApi(); break;
        case 1: *reinterpret_cast<QString*>(_v) = _t->graphicsDriver(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->graphicsProfile(); break;
        case 3: *reinterpret_cast<int*>(_v) = _t->selectedApi(); break;
        case 4: *reinterpret_cast<int*>(_v) = _t->thumbnailSize(); break;
        case 5: *reinterpret_cast<int*>(_v) = _t->gridSize(); break;
        case 6: *reinterpret_cast<int*>(_v) = _t->cacheSizeMB(); break;
        case 7: *reinterpret_cast<int*>(_v) = _t->concurrentThreads(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 3: _t->setSelectedApi(*reinterpret_cast<int*>(_v)); break;
        case 4: _t->setThumbnailSize(*reinterpret_cast<int*>(_v)); break;
        case 5: _t->setGridSize(*reinterpret_cast<int*>(_v)); break;
        case 6: _t->setCacheSizeMB(*reinterpret_cast<int*>(_v)); break;
        case 7: _t->setConcurrentThreads(*reinterpret_cast<int*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *SettingsHelper::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SettingsHelper::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN14SettingsHelperE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SettingsHelper::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}

// SIGNAL 0
void SettingsHelper::graphicsApiChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SettingsHelper::graphicsDriverChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SettingsHelper::graphicsProfileChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SettingsHelper::selectedApiChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SettingsHelper::thumbnailSizeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void SettingsHelper::gridSizeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void SettingsHelper::cacheSizeMBChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void SettingsHelper::concurrentThreadsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}
QT_WARNING_POP
