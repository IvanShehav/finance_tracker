#pragma once

#include "Models.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QVector>

class MarketService : public QObject {
    Q_OBJECT

public:
    explicit MarketService(QObject* parent = nullptr);

public slots:
    void fetchStock(const QString& symbol);
    void fetchCrypto(const QString& symbol);

signals:
    void marketDataLoaded(const QString& title, const QVector<MarketPoint>& points, const QString& message, bool usedFallback);

private:
    QUrl buildStockUrl(const QString& symbol) const;
    QUrl buildCryptoUrl(const QString& symbol) const;
    QVector<MarketPoint> parseStockJson(const QByteArray& data) const;
    QVector<MarketPoint> parseCryptoJson(const QByteArray& data) const;
    QVector<MarketPoint> fallbackSeries(double basePrice) const;
    QString normalizeStockSymbol(const QString& symbol) const;
    QString normalizeCryptoId(const QString& symbol) const;
    QString cryptoDisplayName(const QString& symbol) const;

    QNetworkAccessManager m_network;
};
