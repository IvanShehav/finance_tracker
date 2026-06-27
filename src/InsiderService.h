#pragma once

#include "Models.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QVector>

class InsiderService : public QObject {
    Q_OBJECT

public:
    explicit InsiderService(QObject* parent = nullptr);

public slots:
    void fetchLargeTrades();

signals:
    void tradesLoaded(const QVector<InsiderTrade>& trades, const QString& message, bool usedFallback);

private:
    QUrl buildSecForm4Url() const;
    QVector<InsiderTrade> parseSecAtomFeed(const QByteArray& xmlData) const;
    QVector<InsiderTrade> fallbackTrades() const;
    QString stripHtml(const QString& html) const;

    QNetworkAccessManager m_network;
};
