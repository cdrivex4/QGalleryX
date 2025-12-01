#include <QtQml/qqmlprivate.h>
#include <QtCore/qdir.h>
#include <QtCore/qurl.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

namespace QmlCacheGeneratedCode {
namespace _0x5f_SamsungGalleryTest_resources_qml_MainSemantic_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SamsungGalleryTest_resources_qml_GalleryViewSemantic_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SamsungGalleryTest_resources_qml_GalleryViewTiles_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SamsungGalleryTest_resources_qml_DateScrubber_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SamsungGalleryTest_resources_qml_PhotoViewer_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SamsungGalleryTest_resources_qml_AlbumsView_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SamsungGalleryTest_resources_qml_BottomBar_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SamsungGalleryTest_resources_qml_StatsOverlay_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}
namespace _0x5f_SamsungGalleryTest_resources_qml_UsageGraph_qml { 
    extern const unsigned char qmlData[];
    extern const QQmlPrivate::AOTCompiledFunction aotBuiltFunctions[];
    const QQmlPrivate::CachedQmlUnit unit = {
        reinterpret_cast<const QV4::CompiledData::Unit*>(&qmlData), &aotBuiltFunctions[0], nullptr
    };
}

}
namespace {
struct Registry {
    Registry();
    ~Registry();
    QHash<QString, const QQmlPrivate::CachedQmlUnit*> resourcePathToCachedUnit;
    static const QQmlPrivate::CachedQmlUnit *lookupCachedUnit(const QUrl &url);
};

Q_GLOBAL_STATIC(Registry, unitRegistry)


Registry::Registry() {
    resourcePathToCachedUnit.insert(QStringLiteral("/SamsungGalleryTest/resources/qml/MainSemantic.qml"), &QmlCacheGeneratedCode::_0x5f_SamsungGalleryTest_resources_qml_MainSemantic_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SamsungGalleryTest/resources/qml/GalleryViewSemantic.qml"), &QmlCacheGeneratedCode::_0x5f_SamsungGalleryTest_resources_qml_GalleryViewSemantic_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SamsungGalleryTest/resources/qml/GalleryViewTiles.qml"), &QmlCacheGeneratedCode::_0x5f_SamsungGalleryTest_resources_qml_GalleryViewTiles_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SamsungGalleryTest/resources/qml/DateScrubber.qml"), &QmlCacheGeneratedCode::_0x5f_SamsungGalleryTest_resources_qml_DateScrubber_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SamsungGalleryTest/resources/qml/PhotoViewer.qml"), &QmlCacheGeneratedCode::_0x5f_SamsungGalleryTest_resources_qml_PhotoViewer_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SamsungGalleryTest/resources/qml/AlbumsView.qml"), &QmlCacheGeneratedCode::_0x5f_SamsungGalleryTest_resources_qml_AlbumsView_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SamsungGalleryTest/resources/qml/BottomBar.qml"), &QmlCacheGeneratedCode::_0x5f_SamsungGalleryTest_resources_qml_BottomBar_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SamsungGalleryTest/resources/qml/StatsOverlay.qml"), &QmlCacheGeneratedCode::_0x5f_SamsungGalleryTest_resources_qml_StatsOverlay_qml::unit);
    resourcePathToCachedUnit.insert(QStringLiteral("/SamsungGalleryTest/resources/qml/UsageGraph.qml"), &QmlCacheGeneratedCode::_0x5f_SamsungGalleryTest_resources_qml_UsageGraph_qml::unit);
    QQmlPrivate::RegisterQmlUnitCacheHook registration;
    registration.structVersion = 0;
    registration.lookupCachedQmlUnit = &lookupCachedUnit;
    QQmlPrivate::qmlregister(QQmlPrivate::QmlUnitCacheHookRegistration, &registration);
}

Registry::~Registry() {
    QQmlPrivate::qmlunregister(QQmlPrivate::QmlUnitCacheHookRegistration, quintptr(&lookupCachedUnit));
}

const QQmlPrivate::CachedQmlUnit *Registry::lookupCachedUnit(const QUrl &url) {
    if (url.scheme() != QLatin1String("qrc"))
        return nullptr;
    QString resourcePath = QDir::cleanPath(url.path());
    if (resourcePath.isEmpty())
        return nullptr;
    if (!resourcePath.startsWith(QLatin1Char('/')))
        resourcePath.prepend(QLatin1Char('/'));
    return unitRegistry()->resourcePathToCachedUnit.value(resourcePath, nullptr);
}
}
int QT_MANGLE_NAMESPACE(qInitResources_qmlcache_appSamsungGalleryTest)() {
    ::unitRegistry();
    return 1;
}
Q_CONSTRUCTOR_FUNCTION(QT_MANGLE_NAMESPACE(qInitResources_qmlcache_appSamsungGalleryTest))
int QT_MANGLE_NAMESPACE(qCleanupResources_qmlcache_appSamsungGalleryTest)() {
    return 1;
}
