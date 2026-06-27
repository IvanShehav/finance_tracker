#include "MarketService.h"

#include <QDate>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrlQuery>
#include <QtMath>

MarketService::MarketService(QObject* parent)
    : QObject(parent)
{
}

void MarketService::fetchStock(const QString& symbol)
{
    const QString normalized = normalizeStockSymbol(symbol);
    QNetworkRequest request(buildStockUrl(normalized));
    request.setHeader(QNetworkRequest::UserAgentHeader, "FinanceTrackerQt/1.0");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_network.get(request);
    QTimer::singleShot(8000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, normalized]() {
        const QVector<MarketPoint> points = parseStockJson(reply->readAll());
        if (!points.isEmpty()) {
            emit marketDataLoaded(
                normalized.toUpper(),
                points,
                QStringLiteral("Данные загружены из Yahoo Finance"),
                false
            );
        } else {
            emit marketDataLoaded(
                normalized.toUpper(),
                fallbackSeries(180.0),
                QStringLiteral("API акций недоступен, показан демо-график"),
                true
            );
        }
        reply->deleteLater();
    });
}

void MarketService::fetchCrypto(const QString& symbol)
{
    const QString coinId = normalizeCryptoId(symbol);
    QNetworkRequest request(buildCryptoUrl(coinId));
    request.setHeader(QNetworkRequest::UserAgentHeader, "FinanceTrackerQt/1.0");
    request.setRawHeader("Accept", "application/json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_network.get(request);
    QTimer::singleShot(8000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, symbol, coinId]() {
        const QVector<MarketPoint> points = parseCryptoJson(reply->readAll());
        if (!points.isEmpty()) {
            emit marketDataLoaded(
                cryptoDisplayName(symbol),
                points,
                QStringLiteral("Данные загружены из CoinGecko"),
                false
            );
        } else {
            const double base = coinId == QStringLiteral("bitcoin") ? 65000.0 : 3200.0;
            emit marketDataLoaded(
                cryptoDisplayName(symbol),
                fallbackSeries(base),
                QStringLiteral("API крипты недоступен, показан демо-график"),
                true
            );
        }
        reply->deleteLater();
    });
}

QUrl MarketService::buildStockUrl(const QString& symbol) const
{
    QUrl url(QString("https://query1.finance.yahoo.com/v8/finance/chart/%1").arg(symbol.toUpper()));
    QUrlQuery query;
    query.addQueryItem("range", "1mo");
    query.addQueryItem("interval", "1d");
    url.setQuery(query);
    return url;
}

QUrl MarketService::buildCryptoUrl(const QString& symbol) const
{
    QUrl url(QString("https://api.coingecko.com/api/v3/coins/%1/market_chart").arg(symbol));
    QUrlQuery query;
    query.addQueryItem("vs_currency", "usd");
    query.addQueryItem("days", "30");
    query.addQueryItem("interval", "daily");
    url.setQuery(query);
    return url;
}

QVector<MarketPoint> MarketService::parseStockJson(const QByteArray& data) const
{
    QVector<MarketPoint> points;
    const QJsonDocument document = QJsonDocument::fromJson(data);
    const QJsonObject root = document.object();
    const QJsonArray results = root.value("chart").toObject().value("result").toArray();
    if (results.isEmpty()) {
        return points;
    }

    const QJsonObject result = results.first().toObject();
    const QJsonArray timestamps = result.value("timestamp").toArray();
    const QJsonArray quotes = result.value("indicators").toObject().value("quote").toArray();
    if (quotes.isEmpty()) {
        return points;
    }

    const QJsonArray closes = quotes.first().toObject().value("close").toArray();
    const int count = qMin(timestamps.size(), closes.size());
    for (int i = 0; i < count; ++i) {
        const qint64 seconds = static_cast<qint64>(timestamps.at(i).toDouble());
        const double close = closes.at(i).toDouble(0.0);
        const QDate date = QDateTime::fromSecsSinceEpoch(seconds).date();
        if (date.isValid() && close > 0.0) {
            points.push_back({date, close});
        }
    }
    return points;
}

QVector<MarketPoint> MarketService::parseCryptoJson(const QByteArray& data) const
{
    QVector<MarketPoint> points;
    const QJsonDocument document = QJsonDocument::fromJson(data);
    const QJsonArray prices = document.object().value("prices").toArray();
    for (const QJsonValue& value : prices) {
        const QJsonArray pair = value.toArray();
        if (pair.size() < 2) {
            continue;
        }

        const qint64 milliseconds = static_cast<qint64>(pair.at(0).toDouble());
        const double price = pair.at(1).toDouble();
        const QDate date = QDateTime::fromMSecsSinceEpoch(milliseconds).date();
        if (date.isValid() && price > 0.0) {
            points.push_back({date, price});
        }
    }
    return points;
}

QVector<MarketPoint> MarketService::fallbackSeries(double basePrice) const
{
    QVector<MarketPoint> points;
    const QDate start = QDate::currentDate().addDays(-29);
    for (int i = 0; i < 30; ++i) {
        const double wave = qSin(i / 4.0) * 0.035;
        const double trend = i * 0.0025;
        points.push_back({start.addDays(i), basePrice * (1.0 + wave + trend)});
    }
    return points;
}

QString MarketService::normalizeStockSymbol(const QString& symbol) const
{
    QString normalized = symbol.trimmed().toUpper();
    if (normalized.isEmpty()) {
        normalized = QStringLiteral("AAPL");
    }
    return normalized;
}

QString MarketService::normalizeCryptoId(const QString& symbol) const
{
    const QString normalized = symbol.trimmed().toLower();
    if (normalized == QStringLiteral("btc") || normalized == QStringLiteral("bitcoin") || normalized.isEmpty()) {
        return QStringLiteral("bitcoin");
    }
    if (normalized == QStringLiteral("eth") || normalized == QStringLiteral("ethereum")) {
        return QStringLiteral("ethereum");
    }
    if (normalized == QStringLiteral("sol") || normalized == QStringLiteral("solana")) {
        return QStringLiteral("solana");
    }
    return normalized;
}

QString MarketService::cryptoDisplayName(const QString& symbol) const
{
    const QString normalized = normalizeCryptoId(symbol);
    if (normalized == QStringLiteral("bitcoin")) {
        return QStringLiteral("BTC");
    }
    if (normalized == QStringLiteral("ethereum")) {
        return QStringLiteral("ETH");
    }
    if (normalized == QStringLiteral("solana")) {
        return QStringLiteral("SOL");
    }
    return normalized.toUpper();
}
