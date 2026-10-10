// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Luma Contributors

#include "AudioChannel.h"
#include "AudioEngine.h"
#include "ChannelEffects.h"
#include "MasterBus.h"
#include "PresetManager.h"
#include "SessionManager.h"
#include "TrackLibrary.h"
#include "TrackTimeline.h"
#include "flows/FlowManager.h"
#include "flows/FlowNodeConfig.h"

#include "MockAudioBackend.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QVariantList>
#include <QVariantMap>
#include <QtTest>

class TstFlow : public QObject
{
    Q_OBJECT

private:
    QTemporaryDir m_dir;
    MockAudioBackend* m_backend { nullptr };
    MasterBus* m_master { nullptr };
    TrackLibrary* m_lib { nullptr };
    AudioEngine* m_engine { nullptr };
    PresetManager* m_presets { nullptr };
    FlowManager* m_flows { nullptr };

    QString makeAudioFile(const QString& name)
    {
        const QString path = m_dir.filePath(name);
        QFile f(path);
        [[maybe_unused]] const bool ok = f.open(QIODevice::WriteOnly);
        Q_ASSERT(ok);
        f.close();
        return path;
    }

    QString makePlayFlow(int channelId, const QVariantMap& config = {})
    {
        const QString id = m_flows->createFlow();
        QVariantMap data = m_flows->flowData(id);

        QVariantMap node;
        node[QStringLiteral("id")] = QStringLiteral("n1");
        node[QStringLiteral("channelId")] = channelId;
        node[QStringLiteral("x")] = 0;
        node[QStringLiteral("y")] = 0;
        node[QStringLiteral("config")] = config;
        data[QStringLiteral("nodes")] = QVariantList { node };

        QVariantMap trigger;
        trigger[QStringLiteral("type")] = QStringLiteral("immediate");
        trigger[QStringLiteral("seconds")] = 0;
        QVariantMap edge;
        edge[QStringLiteral("id")] = QStringLiteral("e1");
        edge[QStringLiteral("from")] = QStringLiteral("start");
        edge[QStringLiteral("to")] = QStringLiteral("n1");
        edge[QStringLiteral("trigger")] = trigger;
        edge[QStringLiteral("originAction")] = QStringLiteral("none");
        edge[QStringLiteral("targetAction")] = QStringLiteral("play");
        data[QStringLiteral("edges")] = QVariantList { edge };

        m_flows->updateFlow(id, data);
        return id;
    }

private slots:
    void init()
    {
        m_backend = new MockAudioBackend;
        m_master = new MasterBus(m_backend);
        m_lib = new TrackLibrary;
        m_engine = new AudioEngine(m_backend, m_master, m_lib);
        m_presets = new PresetManager(m_engine, m_master);
        m_flows = new FlowManager(m_engine, m_presets, m_backend);
    }

    void cleanup()
    {
        delete m_flows;
        delete m_presets;
        delete m_engine;
        delete m_lib;
        delete m_master;
        delete m_backend;
    }

    void should_createFlowsWithUniqueIds();
    void should_removeFlow();
    void should_renameFlow_when_nameNotBlank();
    void should_notRename_when_nameBlank();
    void should_storeNodesAndEdges_when_updated();
    void should_roundTripThroughJson();
    void should_continueIdSequence_afterApplyJson();
    void should_trackRunningState();
    void should_stopFlow_when_runningFlowRemoved();
    void should_playChannel_when_flowRuns();
    void should_endOriginAndStartTarget_when_edgeFires();
    void should_restoreChannelState_when_flowStops();
    void should_persistFlowsInSession();
    void should_roundTripNodeConfig();
    void should_completeFlow_when_finishHandoffChainEnds();
};

void TstFlow::should_completeFlow_when_finishHandoffChainEnds()
{
    const QString w0 = makeAudioFile(QStringLiteral("c0.wav"));
    const QString w1 = makeAudioFile(QStringLiteral("c1.wav"));
    const QString w2 = makeAudioFile(QStringLiteral("c2.wav"));
    QVERIFY(m_engine->bindTrack(0, w0));
    QVERIFY(m_engine->bindTrack(1, w1));
    QVERIFY(m_engine->bindTrack(2, w2));

    const QString id = m_flows->createFlow();
    QVariantMap data = m_flows->flowData(id);

    auto makeNode = [](const QString& nid, int ch) {
        QVariantMap cfg;
        cfg[QStringLiteral("fadeSeconds")] = 1;
        cfg[QStringLiteral("fadeMode")] = 2;
        cfg[QStringLiteral("initialVolume")] = 0.0;
        cfg[QStringLiteral("fadeMax")] = 0.5;
        QVariantMap n;
        n[QStringLiteral("id")] = nid;
        n[QStringLiteral("channelId")] = ch;
        n[QStringLiteral("config")] = cfg;
        return n;
    };
    data[QStringLiteral("nodes")] =
        QVariantList { makeNode(QStringLiteral("n1"), 0), makeNode(QStringLiteral("n2"), 1),
                       makeNode(QStringLiteral("n4"), 2) };

    auto trig = [](const QString& type) {
        QVariantMap t;
        t[QStringLiteral("type")] = type;
        t[QStringLiteral("seconds")] = 0;
        return t;
    };
    auto makeEdge = [&](const QString& eid, const QString& from, const QString& to,
                        const QString& type, const QString& origin, const QString& target) {
        QVariantMap e;
        e[QStringLiteral("id")] = eid;
        e[QStringLiteral("from")] = from;
        e[QStringLiteral("to")] = to;
        e[QStringLiteral("trigger")] = trig(type);
        e[QStringLiteral("originAction")] = origin;
        e[QStringLiteral("targetAction")] = target;
        return e;
    };
    data[QStringLiteral("edges")] = QVariantList {
        makeEdge(QStringLiteral("e1"), QStringLiteral("start"), QStringLiteral("n1"),
                 QStringLiteral("immediate"), QStringLiteral("none"), QStringLiteral("play")),
        makeEdge(QStringLiteral("e2"), QStringLiteral("n1"), QStringLiteral("n2"),
                 QStringLiteral("finish"), QStringLiteral("fadeOut"), QStringLiteral("fadeIn")),
        makeEdge(QStringLiteral("e4"), QStringLiteral("n2"), QStringLiteral("n4"),
                 QStringLiteral("finish"), QStringLiteral("fadeOut"), QStringLiteral("fadeIn"))
    };
    m_flows->updateFlow(id, data);

    m_flows->runFlow(id);
    QTRY_VERIFY(m_engine->channelAt(0)->isPlaying());

    emit m_backend->voiceFinished(m_engine->channelAt(0)->voiceId());
    QTRY_VERIFY(m_engine->channelAt(1)->isPlaying());

    emit m_backend->voiceFinished(m_engine->channelAt(1)->voiceId());
    QTRY_VERIFY(m_engine->channelAt(2)->isPlaying());

    QTest::qWait(1400);
    QVERIFY(m_flows->running());

    emit m_backend->voiceFinished(m_engine->channelAt(2)->voiceId());
    QTRY_VERIFY(!m_engine->channelAt(2)->isPlaying());

    QTRY_VERIFY_WITH_TIMEOUT(!m_flows->running(), 3000);
}

void TstFlow::should_createFlowsWithUniqueIds()
{
    QSignalSpy spy(m_flows, &FlowManager::flowsChanged);

    const QString a = m_flows->createFlow();
    const QString b = m_flows->createFlow();

    QCOMPARE(spy.count(), 2);
    QCOMPARE(a, QStringLiteral("flow-1"));
    QCOMPARE(b, QStringLiteral("flow-2"));
    QCOMPARE(m_flows->flows().size(), 2);
    QCOMPARE(m_flows->flows().at(0).toMap().value(QStringLiteral("name")).toString(),
             QStringLiteral("Flow 1"));
}

void TstFlow::should_removeFlow()
{
    const QString a = m_flows->createFlow();
    m_flows->createFlow();

    m_flows->removeFlow(a);

    QCOMPARE(m_flows->flows().size(), 1);
    QCOMPARE(m_flows->flows().at(0).toMap().value(QStringLiteral("id")).toString(),
             QStringLiteral("flow-2"));
}

void TstFlow::should_renameFlow_when_nameNotBlank()
{
    const QString a = m_flows->createFlow();

    m_flows->renameFlow(a, QStringLiteral("  Intro  "));

    QCOMPARE(m_flows->flowData(a).value(QStringLiteral("name")).toString(),
             QStringLiteral("Intro"));
}

void TstFlow::should_notRename_when_nameBlank()
{
    const QString a = m_flows->createFlow();

    m_flows->renameFlow(a, QStringLiteral("   "));

    QCOMPARE(m_flows->flowData(a).value(QStringLiteral("name")).toString(),
             QStringLiteral("Flow 1"));
}

void TstFlow::should_storeNodesAndEdges_when_updated()
{
    const QString id = m_flows->createFlow();

    QVariantMap data = m_flows->flowData(id);
    data[QStringLiteral("preset")] = 3;

    QVariantMap node;
    node[QStringLiteral("id")] = QStringLiteral("n1");
    node[QStringLiteral("channelId")] = 2;
    data[QStringLiteral("nodes")] = QVariantList { node };

    m_flows->updateFlow(id, data);

    const QVariantMap stored = m_flows->flowData(id);
    QCOMPARE(stored.value(QStringLiteral("id")).toString(), id);
    QCOMPARE(stored.value(QStringLiteral("preset")).toInt(), 3);
    QCOMPARE(stored.value(QStringLiteral("nodes")).toList().size(), 1);
    QCOMPARE(m_flows->flows().at(0).toMap().value(QStringLiteral("preset")).toInt(), 3);
}

void TstFlow::should_roundTripThroughJson()
{
    const QString id = m_flows->createFlow();
    m_flows->createFlow();
    QVariantMap data = m_flows->flowData(id);
    data[QStringLiteral("preset")] = 5;
    m_flows->updateFlow(id, data);

    const QJsonArray json = m_flows->toJson();

    FlowManager dst(m_engine, m_presets, m_backend);
    dst.applyJson(json);

    QCOMPARE(dst.flows().size(), 2);
    QCOMPARE(dst.flowData(id).value(QStringLiteral("preset")).toInt(), 5);
    QCOMPARE(dst.flows().at(1).toMap().value(QStringLiteral("id")).toString(),
             QStringLiteral("flow-2"));
}

void TstFlow::should_continueIdSequence_afterApplyJson()
{
    m_flows->createFlow();
    m_flows->createFlow();

    FlowManager dst(m_engine, m_presets, m_backend);
    dst.applyJson(m_flows->toJson());

    QCOMPARE(dst.createFlow(), QStringLiteral("flow-3"));
}

void TstFlow::should_trackRunningState()
{
    const QString wav = makeAudioFile(QStringLiteral("t.wav"));
    QVERIFY(m_engine->bindTrack(0, wav));
    const QString id = makePlayFlow(0);
    QSignalSpy spy(m_flows, &FlowManager::runningChanged);

    QVERIFY(!m_flows->running());
    m_flows->runFlow(id);
    QVERIFY(m_flows->running());
    QCOMPARE(m_flows->runningFlowId(), id);
    QVERIFY(m_flows->activeChannels().contains(QVariant(0)));

    m_flows->stopFlow();
    QVERIFY(!m_flows->running());
    QVERIFY(m_flows->activeChannels().isEmpty());
    QCOMPARE(spy.count(), 2);
}

void TstFlow::should_stopFlow_when_runningFlowRemoved()
{
    const QString wav = makeAudioFile(QStringLiteral("t.wav"));
    QVERIFY(m_engine->bindTrack(0, wav));
    const QString id = makePlayFlow(0);
    m_flows->runFlow(id);
    QVERIFY(m_flows->running());

    m_flows->removeFlow(id);

    QVERIFY(!m_flows->running());
    QVERIFY(m_flows->runningFlowId().isEmpty());
}

void TstFlow::should_playChannel_when_flowRuns()
{
    const QString wav = makeAudioFile(QStringLiteral("t.wav"));
    QVERIFY(m_engine->bindTrack(0, wav));
    const QString id = makePlayFlow(0);

    m_flows->runFlow(id);
    QTRY_VERIFY(m_engine->channelAt(0)->isPlaying());
    QVERIFY(m_flows->running());
}

void TstFlow::should_endOriginAndStartTarget_when_edgeFires()
{
    const QString wav0 = makeAudioFile(QStringLiteral("a.wav"));
    const QString wav1 = makeAudioFile(QStringLiteral("b.wav"));
    QVERIFY(m_engine->bindTrack(0, wav0));
    QVERIFY(m_engine->bindTrack(1, wav1));

    const QString id = m_flows->createFlow();
    QVariantMap data = m_flows->flowData(id);

    QVariantMap n1;
    n1[QStringLiteral("id")] = QStringLiteral("n1");
    n1[QStringLiteral("channelId")] = 0;
    n1[QStringLiteral("config")] = QVariantMap();
    QVariantMap n2;
    n2[QStringLiteral("id")] = QStringLiteral("n2");
    n2[QStringLiteral("channelId")] = 1;
    n2[QStringLiteral("config")] = QVariantMap();
    data[QStringLiteral("nodes")] = QVariantList { n1, n2 };

    QVariantMap immediate;
    immediate[QStringLiteral("type")] = QStringLiteral("immediate");
    immediate[QStringLiteral("seconds")] = 0;

    QVariantMap e1;
    e1[QStringLiteral("id")] = QStringLiteral("e1");
    e1[QStringLiteral("from")] = QStringLiteral("start");
    e1[QStringLiteral("to")] = QStringLiteral("n1");
    e1[QStringLiteral("trigger")] = immediate;
    e1[QStringLiteral("originAction")] = QStringLiteral("none");
    e1[QStringLiteral("targetAction")] = QStringLiteral("play");

    QVariantMap e2;
    e2[QStringLiteral("id")] = QStringLiteral("e2");
    e2[QStringLiteral("from")] = QStringLiteral("n1");
    e2[QStringLiteral("to")] = QStringLiteral("n2");
    e2[QStringLiteral("trigger")] = immediate;
    e2[QStringLiteral("originAction")] = QStringLiteral("stop");
    e2[QStringLiteral("targetAction")] = QStringLiteral("play");

    data[QStringLiteral("edges")] = QVariantList { e1, e2 };
    m_flows->updateFlow(id, data);

    m_flows->runFlow(id);
    QTRY_VERIFY(m_engine->channelAt(1)->isPlaying());
    QVERIFY(!m_engine->channelAt(0)->isPlaying());
}

void TstFlow::should_restoreChannelState_when_flowStops()
{
    const QString wav = makeAudioFile(QStringLiteral("t.wav"));
    QVERIFY(m_engine->bindTrack(0, wav));
    m_engine->setVolume(0, 0.5f);

    QVariantMap config;
    config[QStringLiteral("initialVolume")] = 0.2;
    const QString id = makePlayFlow(0, config);

    m_flows->runFlow(id);
    QTRY_VERIFY(m_engine->channelAt(0)->isPlaying());

    m_flows->stopFlow();
    QVERIFY(!m_engine->channelAt(0)->isPlaying());
    QVERIFY(qAbs(m_engine->channelAt(0)->volume() - 0.5f) < 1e-3f);
}

void TstFlow::should_persistFlowsInSession()
{
    const QString sessionPath = m_dir.filePath(QStringLiteral("flows.json"));

    {
        MockAudioBackend backend;
        MasterBus master(&backend);
        TrackLibrary lib;
        AudioEngine engine(&backend, &master, &lib);
        PresetManager presets(&engine, &master);
        FlowManager flows(&engine, &presets, &backend);
        SessionManager session(&lib, &presets, sessionPath);
        session.setFlowManager(&flows);

        const QString id = flows.createFlow();
        QVariantMap data = flows.flowData(id);
        data[QStringLiteral("preset")] = 4;
        QVariantMap node;
        node[QStringLiteral("id")] = QStringLiteral("n1");
        node[QStringLiteral("channelId")] = 1;
        data[QStringLiteral("nodes")] = QVariantList { node };
        flows.updateFlow(id, data);

        QVERIFY(session.save());
    }

    {
        MockAudioBackend backend;
        MasterBus master(&backend);
        TrackLibrary lib;
        AudioEngine engine(&backend, &master, &lib);
        PresetManager presets(&engine, &master);
        FlowManager flows(&engine, &presets, &backend);
        SessionManager session(&lib, &presets, sessionPath);
        session.setFlowManager(&flows);

        QVERIFY(session.load());

        QCOMPARE(flows.flows().size(), 1);
        const QVariantMap d = flows.flowData(QStringLiteral("flow-1"));
        QCOMPARE(d.value(QStringLiteral("preset")).toInt(), 4);
        QCOMPARE(d.value(QStringLiteral("nodes")).toList().size(), 1);
    }
}

void TstFlow::should_roundTripNodeConfig()
{
    FlowNodeConfig cfg(m_backend);
    cfg.setPan(-0.5f);
    cfg.setInitialVolume(0.3f);
    cfg.setFadeMax(0.9f);
    cfg.setFadeMin(0.1f);
    cfg.setFadeSeconds(7);
    cfg.setFadeMode(3);
    cfg.setLoop(true);
    cfg.setStartSeconds(12.0);
    cfg.effects()->setReverbEnabled(true);
    cfg.effects()->setReverbMix(0.42f);
    cfg.timeline()->setStart(0.25);

    const QVariantMap map = cfg.toMap();

    FlowNodeConfig restored(m_backend);
    restored.loadFromMap(map);

    QVERIFY(qAbs(restored.pan() - (-0.5f)) < 1e-4f);
    QVERIFY(qAbs(restored.initialVolume() - 0.3f) < 1e-4f);
    QVERIFY(qAbs(restored.fadeMax() - 0.9f) < 1e-4f);
    QVERIFY(qAbs(restored.fadeMin() - 0.1f) < 1e-4f);
    QCOMPARE(restored.fadeSeconds(), 7);
    QCOMPARE(restored.fadeMode(), 3);
    QVERIFY(restored.loop());
    QVERIFY(qAbs(restored.startSeconds() - 12.0) < 1e-6);
    QVERIFY(restored.effects()->reverbEnabled());
    QVERIFY(qAbs(restored.effects()->reverbMix() - 0.42f) < 1e-4f);
    QVERIFY(restored.timeline()->hasStart());
    QVERIFY(qAbs(restored.timeline()->start() - 0.25) < 1e-6);
}

QTEST_MAIN(TstFlow)
#include "tst_flow.moc"
