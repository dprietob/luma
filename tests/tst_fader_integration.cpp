// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "MockAudioBackend.h"

#include "AudioEngine.h"
#include "MasterBus.h"
#include "TrackLibrary.h"

#include <QtQuickTest/quicktest.h>

#include <QDir>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTemporaryFile>

class FaderIntegrationSetup : public QObject
{
    Q_OBJECT

public:
    FaderIntegrationSetup()
    {
        m_file.setFileTemplate(QDir::tempPath() + "/lmfi_XXXXXX.wav");
        const bool opened = m_file.open();
        Q_ASSERT(opened);
        Q_UNUSED(opened)
        m_file.write("data");
        m_file.flush();
    }

public slots:
    void qmlEngineAvailable(QQmlEngine* engine)
    {
        engine->rootContext()->setContextProperty(QStringLiteral("audioEngine"), &m_engine);
        engine->rootContext()->setContextProperty(QStringLiteral("testFile"), m_file.fileName());
    }

private:
    MockAudioBackend m_backend;
    MasterBus m_master { &m_backend };
    TrackLibrary m_library;
    AudioEngine m_engine { &m_backend, &m_master, &m_library };
    QTemporaryFile m_file;
};

QUICK_TEST_MAIN_WITH_SETUP(fader_integration, FaderIntegrationSetup)

#include "tst_fader_integration.moc"
