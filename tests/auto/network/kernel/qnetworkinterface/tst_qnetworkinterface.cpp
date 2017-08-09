/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>

#include <qcoreapplication.h>
#include <qnetworkinterface.h>
#include <qudpsocket.h>
#ifndef QT_NO_BEARERMANAGEMENT
#include <QNetworkConfigurationManager>
#include <QNetworkSession>
#endif
#include "../../../network-settings.h"
#include "emulationdetector.h"

class tst_QNetworkInterface : public QObject
{
    Q_OBJECT

public:
    tst_QNetworkInterface();
    virtual ~tst_QNetworkInterface();

    bool isIPv6Working();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void dump();
    void consistencyCheck();
    void loopbackIPv4();
    void loopbackIPv6();
    void localAddress();
    void interfaceFromXXX_data();
    void interfaceFromXXX();
    void copyInvalidInterface();

private:
#ifndef QT_NO_BEARERMANAGEMENT
    QNetworkConfigurationManager *netConfMan;
    QNetworkConfiguration networkConfiguration;
    QScopedPointer<QNetworkSession> networkSession;
#endif
};

tst_QNetworkInterface::tst_QNetworkInterface()
{
}

tst_QNetworkInterface::~tst_QNetworkInterface()
{
}

bool tst_QNetworkInterface::isIPv6Working()
{
    QUdpSocket socket;
    socket.connectToHost(QHostAddress::LocalHostIPv6, 1234);
    return socket.state() == QAbstractSocket::ConnectedState || socket.waitForConnected(100);
}

void tst_QNetworkInterface::initTestCase()
{
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No network test server available");
#ifndef QT_NO_BEARERMANAGEMENT
    netConfMan = new QNetworkConfigurationManager(this);
    if (netConfMan->capabilities()
            & QNetworkConfigurationManager::NetworkSessionRequired) {
        networkConfiguration = netConfMan->defaultConfiguration();
        networkSession.reset(new QNetworkSession(networkConfiguration));
        if (!networkSession->isOpen()) {
            networkSession->open();
            QVERIFY(networkSession->waitForOpened(30000));
        }
    }
#endif
}

void tst_QNetworkInterface::cleanupTestCase()
{
#ifndef QT_NO_BEARERMANAGEMENT
    if (networkSession && networkSession->isOpen()) {
        networkSession->close();
    }
#endif
}

void tst_QNetworkInterface::dump()
{
    // This is for manual testing:
    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    foreach (const QNetworkInterface &i, allInterfaces) {
        QString flags;
        if (i.flags() & QNetworkInterface::IsUp) flags += "Up,";
        if (i.flags() & QNetworkInterface::IsRunning) flags += "Running,";
        if (i.flags() & QNetworkInterface::CanBroadcast) flags += "Broadcast,";
        if (i.flags() & QNetworkInterface::IsLoopBack) flags += "Loopback,";
        if (i.flags() & QNetworkInterface::IsPointToPoint) flags += "PointToPoint,";
        if (i.flags() & QNetworkInterface::CanMulticast) flags += "Multicast,";
        flags.chop(1);          // drop last comma

        QString friendlyName = i.humanReadableName();
        if (friendlyName != i.name()) {
            friendlyName.prepend('(');
            friendlyName.append(')');
        } else {
            friendlyName.clear();
        }
        qDebug() << "Interface:     " << i.name() << qPrintable(friendlyName);
        QVERIFY(i.isValid());

        qDebug() << "    index:     " << i.index();
        qDebug() << "    flags:     " << qPrintable(flags);
        qDebug() << "    type:      " << i.type();
        qDebug() << "    hw address:" << qPrintable(i.hardwareAddress());

        int count = 0;
        foreach (const QNetworkAddressEntry &e, i.addressEntries()) {
            QDebug s = qDebug();
            s.nospace() <<    "    address "
                        << qSetFieldWidth(2) << count++ << qSetFieldWidth(0);
            s.nospace() << ": " << qPrintable(e.ip().toString());
            if (!e.netmask().isNull())
                s.nospace() << '/' << e.prefixLength()
                            << " (" << qPrintable(e.netmask().toString()) << ')';
            if (!e.broadcast().isNull())
                s.nospace() << " broadcast " << qPrintable(e.broadcast().toString());
            if (e.dnsEligibility() == QNetworkAddressEntry::DnsEligible)
                s.nospace() << " dns-eligible";
            else if (e.dnsEligibility() == QNetworkAddressEntry::DnsIneligible)
                s.nospace() << " dns-ineligible";
            if (e.isLifetimeKnown()) {
#define printable(l) qPrintable(l.isForever() ? "forever" : QString::fromLatin1("%1ms").arg(l.remainingTime()))
                s.nospace() << " preferred:" << printable(e.preferredLifetime())
                            << " valid:" << printable(e.validityLifetime());
#undef printable
            }
        }
    }
}

void tst_QNetworkInterface::consistencyCheck()
{
    QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
    QSet<QString> interfaceNames;
    QVector<int> interfaceIndexes;

    foreach (const QNetworkInterface &iface, ifaces) {
        QVERIFY(iface.isValid());
        QVERIFY2(!interfaceNames.contains(iface.name()),
                 "duplicate name = " + iface.name().toLocal8Bit());
        interfaceNames << iface.name();

        QVERIFY2(!interfaceIndexes.contains(iface.index()),
                 "duplicate index = " + QByteArray::number(iface.index()));
        if (iface.index())
            interfaceIndexes << iface.index();

        const QList<QNetworkAddressEntry> addresses = iface.addressEntries();
        for (auto entry : addresses) {
            QVERIFY(entry.ip().protocol() != QAbstractSocket::UnknownNetworkLayerProtocol);
            if (!entry.preferredLifetime().isForever() || !entry.validityLifetime().isForever())
                QVERIFY(entry.isLifetimeKnown());
            if (!entry.validityLifetime().isForever())
                QVERIFY(entry.isTemporary());
        }
    }
}

void tst_QNetworkInterface::loopbackIPv4()
{
    QList<QHostAddress> all = QNetworkInterface::allAddresses();
    QVERIFY(all.contains(QHostAddress(QHostAddress::LocalHost)));
}

void tst_QNetworkInterface::loopbackIPv6()
{
    if (!isIPv6Working())
        QSKIP("IPv6 not active on this machine");
    QList<QHostAddress> all = QNetworkInterface::allAddresses();
    QVERIFY(all.contains(QHostAddress(QHostAddress::LocalHostIPv6)));
}

void tst_QNetworkInterface::localAddress()
{
    QUdpSocket socket;
    socket.connectToHost(QtNetworkSettings::serverName(), 80);
    QVERIFY(socket.waitForConnected(5000));

    QHostAddress local = socket.localAddress();

    // test that we can find the address that QUdpSocket reported
    QList<QHostAddress> all = QNetworkInterface::allAddresses();
    QVERIFY(all.contains(local));
}

void tst_QNetworkInterface::interfaceFromXXX_data()
{
    QTest::addColumn<QNetworkInterface>("iface");

    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    foreach (QNetworkInterface iface, allInterfaces)
        QTest::newRow(iface.name().toLocal8Bit()) << iface;
}

void tst_QNetworkInterface::interfaceFromXXX()
{
    QFETCH(QNetworkInterface, iface);

    QVERIFY(QNetworkInterface::interfaceFromName(iface.name()).isValid());
    if (int idx = iface.index()) {
        QVERIFY(QNetworkInterface::interfaceFromIndex(idx).isValid());
        if (EmulationDetector::isRunningArmOnX86())
            QEXPECT_FAIL("", "SIOCGIFNAME fails on QEMU", Continue);
        QCOMPARE(QNetworkInterface::interfaceNameFromIndex(idx), iface.name());
        QCOMPARE(QNetworkInterface::interfaceIndexFromName(iface.name()), idx);
    }
    foreach (QNetworkAddressEntry entry, iface.addressEntries()) {
        QVERIFY(!entry.ip().isNull());

        if (!entry.netmask().isNull()) {
            QCOMPARE(entry.netmask().protocol(), entry.ip().protocol());

            // if the netmask is known, the broadcast is known
            // but only for IPv4 (there is no such thing as broadcast in IPv6)
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                QVERIFY(!entry.broadcast().isNull());
            }
        }

        if (!entry.broadcast().isNull())
            QCOMPARE(entry.broadcast().protocol(), entry.ip().protocol());
    }
}

void tst_QNetworkInterface::copyInvalidInterface()
{
    // Force a copy of an interfaces that isn't likely to exist
    QNetworkInterface i = QNetworkInterface::interfaceFromName("plopp");
    QVERIFY(!i.isValid());

    QCOMPARE(i.index(), 0);
    QVERIFY(i.name().isEmpty());
    QVERIFY(i.humanReadableName().isEmpty());
    QVERIFY(i.hardwareAddress().isEmpty());
    QCOMPARE(int(i.flags()), 0);
    QVERIFY(i.addressEntries().isEmpty());
}

QTEST_MAIN(tst_QNetworkInterface)
#include "tst_qnetworkinterface.moc"
