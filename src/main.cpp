// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "engine/AppSettings.h"
#include "engine/AudioChannel.h"
#include "engine/AudioDeviceManager.h"
#include "engine/AudioEngine.h"
#include "engine/ChannelEffects.h"
#include "engine/MasterBus.h"
#include "engine/PresetManager.h"
#include "engine/ProjectManager.h"
#include "engine/SessionManager.h"
#include "engine/TrackLibrary.h"
#include "engine/TrackTimeline.h"
#include "engine/backend/PortAudioBackend.h"
#include "engine/flows/FlowManager.h"
#include "engine/flows/FlowNodeConfig.h"

#include <QFileInfo>
#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTimer>
#include <QTranslator>
#include <QtQml>

#include <memory>

namespace {
constexpr char k_QmlUri[] = "Luma";
}

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Luma"));
    QGuiApplication::setOrganizationName(QStringLiteral("Luma"));
    QGuiApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.dprietob.luma"));
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    // --- Infraestructura (Motor de audio) ---
    auto backend = std::make_unique<PortAudioBackend>();
    if (!backend->initialize()) {
        qWarning("Luma: audio backend failed to initialize; continuing (UI only).");
    }

    // --- Capa de negocio (DI por constructor) ---
    auto masterBus = std::make_unique<MasterBus>(backend.get());
    auto trackLibrary = std::make_unique<TrackLibrary>();
    auto audioEngine =
        std::make_unique<AudioEngine>(backend.get(), masterBus.get(), trackLibrary.get());
    auto presetManager = std::make_unique<PresetManager>(audioEngine.get(), masterBus.get());
    auto sessionManager = std::make_unique<SessionManager>(trackLibrary.get(), presetManager.get());
    auto audioDevices = std::make_unique<AudioDeviceManager>(backend.get());
    auto flowManager =
        std::make_unique<FlowManager>(audioEngine.get(), presetManager.get(), backend.get());
    auto flowNodeConfig = std::make_unique<FlowNodeConfig>(backend.get());
    auto appSettings = std::make_unique<AppSettings>();
    auto projectManager = std::make_unique<ProjectManager>(sessionManager.get(), trackLibrary.get(),
                                                           appSettings.get());
    sessionManager->setAudioDeviceManager(audioDevices.get());
    sessionManager->setFlowManager(flowManager.get());

    auto translator = std::make_unique<QTranslator>();
    const QString languagePref = appSettings->language();
    const QString localeName = (languagePref.isEmpty() || languagePref == QStringLiteral("system"))
                                   ? QLocale::system().name()
                                   : languagePref;
    if (translator->load(QStringLiteral("luma_%1").arg(localeName), QStringLiteral(":/i18n")) ||
        translator->load(QStringLiteral("luma_%1").arg(localeName.left(2)),
                         QStringLiteral(":/i18n"))) {
        QCoreApplication::installTranslator(translator.get());
    }

    backend->setDecodeCacheBytes(static_cast<qint64>(appSettings->audioCacheMB()) * 1024 * 1024);
    QObject::connect(appSettings.get(), &AppSettings::audioCacheMBChanged, &app, [&]() {
        backend->setDecodeCacheBytes(static_cast<qint64>(appSettings->audioCacheMB()) * 1024 *
                                     1024);
    });

    const QStringList args = QGuiApplication::arguments();
    const QString openArg = args.size() > 1 ? args.at(1) : QString();
    QString startupProject;
    if (!openArg.isEmpty() &&
        QFileInfo(openArg).suffix().compare(QStringLiteral("luma"), Qt::CaseInsensitive) == 0 &&
        QFileInfo::exists(openArg)) {
        startupProject = openArg;
    } else if (appSettings->openLastOnStartup()) {
        const QString last = appSettings->lastProject();
        if (!last.isEmpty() && QFileInfo::exists(last)) startupProject = last;
    }
    if (!startupProject.isEmpty()) projectManager->setLoading(true);

    QObject::connect(projectManager.get(), &ProjectManager::projectChanged, &app, [&]() {
        if (!projectManager->hasProject()) audioEngine->stopAll();
    });

    // --- Registro de tipos en QML (uncreatable: instancias vienen de C++) ---
    qmlRegisterUncreatableType<AudioEngine>(k_QmlUri, 1, 0, "AudioEngine",
                                            QStringLiteral("AudioEngine se inyecta desde C++"));
    qmlRegisterUncreatableType<AudioChannel>(
        k_QmlUri, 1, 0, "AudioChannel",
        QStringLiteral("AudioChannel se obtiene de AudioEngine.channels"));
    qmlRegisterUncreatableType<ChannelEffects>(
        k_QmlUri, 1, 0, "ChannelEffects",
        QStringLiteral("ChannelEffects se obtiene de AudioChannel.effects"));
    qmlRegisterUncreatableType<TrackTimeline>(
        k_QmlUri, 1, 0, "TrackTimeline",
        QStringLiteral("TrackTimeline se obtiene de AudioChannel.timeline"));
    qmlRegisterUncreatableType<MasterBus>(k_QmlUri, 1, 0, "MasterBus",
                                          QStringLiteral("MasterBus se inyecta desde C++"));
    qmlRegisterUncreatableType<TrackLibrary>(k_QmlUri, 1, 0, "TrackLibrary",
                                             QStringLiteral("TrackLibrary se inyecta desde C++"));
    qmlRegisterUncreatableType<PresetManager>(k_QmlUri, 1, 0, "PresetManager",
                                              QStringLiteral("PresetManager se inyecta desde C++"));
    qmlRegisterUncreatableType<FlowNodeConfig>(
        k_QmlUri, 1, 0, "FlowNodeConfig", QStringLiteral("FlowNodeConfig se inyecta desde C++"));
    qmlRegisterUncreatableType<ProjectManager>(
        k_QmlUri, 1, 0, "ProjectManager", QStringLiteral("ProjectManager se inyecta desde C++"));

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("audioEngine"), audioEngine.get());
    engine.rootContext()->setContextProperty(QStringLiteral("masterBus"), masterBus.get());
    engine.rootContext()->setContextProperty(QStringLiteral("trackLibrary"), trackLibrary.get());
    engine.rootContext()->setContextProperty(QStringLiteral("presetManager"), presetManager.get());
    engine.rootContext()->setContextProperty(QStringLiteral("audioDevices"), audioDevices.get());
    engine.rootContext()->setContextProperty(QStringLiteral("flowManager"), flowManager.get());
    engine.rootContext()->setContextProperty(QStringLiteral("flowNodeConfig"),
                                             flowNodeConfig.get());
    engine.rootContext()->setContextProperty(QStringLiteral("projectManager"),
                                             projectManager.get());
    engine.rootContext()->setContextProperty(QStringLiteral("sessionManager"),
                                             sessionManager.get());
    engine.rootContext()->setContextProperty(QStringLiteral("appSettings"), appSettings.get());

    QObject::connect(&app, &QGuiApplication::aboutToQuit, [&]() {
        projectManager->save();
        backend->shutdown();
    });

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule(k_QmlUri, QStringLiteral("Main"));
    if (engine.rootObjects().isEmpty()) return -1;

    if (!startupProject.isEmpty()) {
        ProjectManager* pm = projectManager.get();
        auto done = std::make_shared<bool>(false);
        auto open = [done, pm, startupProject]() {
            if (*done) return;
            *done = true;
            pm->openProject(startupProject);
        };
        if (auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst())) {
            auto conn = std::make_shared<QMetaObject::Connection>();
            *conn = QObject::connect(window, &QQuickWindow::frameSwapped, pm, [conn, open]() {
                QObject::disconnect(*conn);
                open();
            });
        }
        QTimer::singleShot(1000, pm, open);
    }

    return app.exec();
}
