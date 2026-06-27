#pragma once

#include <QDate>
#include <QJsonObject>
#include <QString>
#include <QStringList>

struct Expense {
    QDate date;
    QString category;
    QString title;
    double amount = 0.0;

    QJsonObject toJson() const;
    static Expense fromJson(const QJsonObject& object);
};

struct CategoryBudget {
    QString category;
    double limit = 0.0;
    double spent = 0.0;

    double remaining() const;
    QJsonObject toJson() const;
    static CategoryBudget fromJson(const QJsonObject& object);
};

struct Asset {
    QString type;
    QString name;
    double quantity = 0.0;
    double price = 0.0;

    double marketValue() const;
    QJsonObject toJson() const;
    static Asset fromJson(const QJsonObject& object);
};

struct InsiderTrade {
    QString filingDate;
    QString tradeDate;
    QString ticker;
    QString company;
    QString insider;
    QString role;
    QString tradeType;
    QString price;
    QString value;
};

struct MarketPoint {
    QDate date;
    double price = 0.0;
};

QStringList defaultBudgetCategories();
QStringList defaultAssetTypes();
