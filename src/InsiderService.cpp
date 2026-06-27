#include "InsiderService.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QHash>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTimer>
#include <QUrlQuery>
#include <QXmlStreamReader>

InsiderService::InsiderService(QObject* parent)
    : QObject(parent)
{
}

void InsiderService::fetchLargeTrades()
{
    QNetworkRequest request(buildSecForm4Url());
    request.setHeader(QNetworkRequest::UserAgentHeader, "FinanceTrackerQt student-project contact@example.com");
    request.setRawHeader("Accept", "application/atom+xml,application/xml,text/xml,*/*");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_network.get(request);
    QTimer::singleShot(8000, reply, [reply]() {
        if (reply->isRunning()) {
            reply->abort();
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray payload = reply->readAll();
        const QVector<InsiderTrade> trades = parseSecAtomFeed(payload);
        if (!trades.isEmpty()) {
            emit tradesLoaded(
                trades,
                QStringLiteral("Загружены свежие инсайдерские Form 4 из SEC EDGAR"),
                false
            );
        } else {
            emit tradesLoaded(
                fallbackTrades(),
                QStringLiteral("Сеть или парсинг недоступны, показаны демо-данные"),
                true
            );
        }
        reply->deleteLater();
    });
}

QUrl InsiderService::buildSecForm4Url() const
{
    QUrl url("https://www.sec.gov/cgi-bin/browse-edgar");
    QUrlQuery query;
    query.addQueryItem("action", "getcurrent");
    query.addQueryItem("type", "4");
    query.addQueryItem("owner", "include");
    query.addQueryItem("count", "80");
    query.addQueryItem("output", "atom");
    url.setQuery(query);
    return url;
}

QVector<InsiderTrade> InsiderService::parseSecAtomFeed(const QByteArray& xmlData) const
{
    struct SecEntry {
        QString accession;
        QString filingDate;
        QString updatedDate;
        QString issuer;
        QString issuerCik;
        QString reporter;
    };

    auto extractAccession = [](const QString& text) {
        const QRegularExpression pattern("(?:accession-number=|AccNo:\\s*)([0-9-]+)");
        const QRegularExpressionMatch match = pattern.match(text);
        return match.hasMatch() ? match.captured(1) : QString();
    };

    auto extractFiledDate = [](const QString& summary) {
        const QRegularExpression pattern("Filed:\\s*(\\d{4}-\\d{2}-\\d{2})");
        const QRegularExpressionMatch match = pattern.match(summary);
        return match.hasMatch() ? match.captured(1) : QString();
    };

    auto extractRole = [](const QString& title) {
        const QRegularExpression pattern("\\((Issuer|Reporting)\\)\\s*$", QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch match = pattern.match(title);
        return match.hasMatch() ? match.captured(1) : QString();
    };

    auto extractCik = [](const QString& title) {
        const QRegularExpression pattern("\\((\\d{10})\\)");
        const QRegularExpressionMatch match = pattern.match(title);
        return match.hasMatch() ? match.captured(1) : QString();
    };

    auto extractName = [](QString title) {
        title.remove(QRegularExpression("^\\s*4\\s*-\\s*"));
        title.remove(QRegularExpression("\\s*\\(\\d{10}\\)"));
        title.remove(QRegularExpression("\\s*\\((Issuer|Reporting)\\)\\s*$", QRegularExpression::CaseInsensitiveOption));
        return title.simplified();
    };

    QHash<QString, SecEntry> entries;
    QVector<QString> order;

    QXmlStreamReader xml(xmlData);
    bool inEntry = false;
    QString title;
    QString summary;
    QString updated;
    QString categoryTerm;
    QString id;

    auto flushEntry = [&]() {
        if (categoryTerm != QStringLiteral("4")) {
            return;
        }

        QString accession = extractAccession(id);
        if (accession.isEmpty()) {
            accession = extractAccession(summary);
        }
        if (accession.isEmpty()) {
            accession = title + updated;
        }

        if (!entries.contains(accession)) {
            order.push_back(accession);
        }

        SecEntry& entry = entries[accession];
        entry.accession = accession;
        entry.updatedDate = updated.left(10);
        entry.filingDate = extractFiledDate(stripHtml(summary));

        const QString role = extractRole(title);
        const QString name = extractName(title);
        if (role.compare(QStringLiteral("Issuer"), Qt::CaseInsensitive) == 0) {
            entry.issuer = name;
            entry.issuerCik = extractCik(title);
        } else if (role.compare(QStringLiteral("Reporting"), Qt::CaseInsensitive) == 0) {
            entry.reporter = name;
        } else if (entry.reporter.isEmpty()) {
            entry.reporter = name;
        }
    };

    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name() == QStringLiteral("entry")) {
            inEntry = true;
            title.clear();
            summary.clear();
            updated.clear();
            categoryTerm.clear();
            id.clear();
            continue;
        }

        if (xml.isEndElement() && xml.name() == QStringLiteral("entry")) {
            flushEntry();
            inEntry = false;
            continue;
        }

        if (!inEntry || !xml.isStartElement()) {
            continue;
        }

        const QStringView name = xml.name();
        if (name == QStringLiteral("title")) {
            title = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
        } else if (name == QStringLiteral("summary")) {
            summary = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
        } else if (name == QStringLiteral("updated")) {
            updated = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
        } else if (name == QStringLiteral("id")) {
            id = xml.readElementText(QXmlStreamReader::IncludeChildElements).simplified();
        } else if (name == QStringLiteral("category")) {
            categoryTerm = xml.attributes().value(QStringLiteral("term")).toString();
        }
    }

    QVector<InsiderTrade> trades;
    for (const QString& key : order) {
        const SecEntry entry = entries.value(key);
        if (entry.issuer.isEmpty() && entry.reporter.isEmpty()) {
            continue;
        }

        InsiderTrade trade;
        trade.filingDate = entry.filingDate.isEmpty() ? entry.updatedDate : entry.filingDate;
        trade.tradeDate = entry.updatedDate;
        trade.ticker = entry.issuerCik.isEmpty() ? QStringLiteral("SEC") : QStringLiteral("CIK ") + entry.issuerCik;
        trade.company = entry.issuer.isEmpty() ? QStringLiteral("See filing") : entry.issuer;
        trade.insider = entry.reporter.isEmpty() ? QStringLiteral("See filing") : entry.reporter;
        trade.role = QStringLiteral("Reporting owner");
        trade.tradeType = QStringLiteral("Form 4");
        trade.price = QStringLiteral("EDGAR");
        trade.value = entry.accession.startsWith(QStringLiteral("000"))
            ? QStringLiteral("Acc. ") + entry.accession
            : QStringLiteral("Recent filing");
        trades.push_back(trade);

        if (trades.size() >= 30) {
            break;
        }
    }

    return trades;
}

QVector<InsiderTrade> InsiderService::fallbackTrades() const
{
    return {
        {
            "2026-06-20", "2026-06-18", "MSFT", "Microsoft Corp",
            "Demo Insider", "Director", "Purchase", "$420.10", "$2,100,500"
        },
        {
            "2026-06-18", "2026-06-17", "NVDA", "NVIDIA Corp",
            "Demo Officer", "Chief Officer", "Sale", "$144.30", "$3,450,000"
        },
        {
            "2026-06-17", "2026-06-16", "AAPL", "Apple Inc",
            "Demo Fund", "10% Owner", "Purchase", "$198.90", "$5,260,000"
        }
    };
}

QString InsiderService::stripHtml(const QString& html) const
{
    QTextDocument document;
    document.setHtml(html);
    return document.toPlainText().simplified();
}
